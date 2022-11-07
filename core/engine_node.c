#include "engine_node.h"

#include "engine.h"

#include "memory.h"

STATIC_ASSERT(sizeof(EngineNode) == 72);

static void freeMemory(void* ptr) {
	EngineNode* node = (EngineNode*)ptr;
	//SDL stuff
	SDL_DestroyTexture(node->texture);

	//free this node type's memory
	FREE(EngineNode, ptr);
}

void initEngineNode(EngineNode* node, Interpreter* interpreter, void* tb, size_t size) {
	//init
	node->freeMemory = freeMemory;
	node->functions = ALLOCATE(LiteralDictionary, 1);
	node->parent = NULL;
	node->tag = OPAQUE_TAG_ENGINE_NODE;
	node->children = NULL;
	node->capacity = 0;
	node->count = 0;
	node->texture = NULL;

	initLiteralDictionary(node->functions);

	//run bytecode
	runInterpreter(interpreter, tb, size);

	//grab all top-level functions from the dirty interpreter
	LiteralDictionary* variablesPtr = &interpreter->scope->variables;

	for (int i = 0; i < variablesPtr->capacity; i++) {
		//skip empties and tombstones
		if (IS_NULL(variablesPtr->entries[i].key)) {
			continue;
		}

		//if this variable is a function
		_entry* entry = &variablesPtr->entries[i];
		if (IS_FUNCTION(entry->value)) {
			//save a copy
			setLiteralDictionary(node->functions, entry->key, entry->value);
		}
	}
}

void pushEngineNode(EngineNode* node, EngineNode* child) {
	//push to the array (prune tombstones when expanding/copying)
	if (node->count + 1 > node->capacity) {
		int oldCapacity = node->capacity;

		node->capacity = GROW_CAPACITY(oldCapacity);
		node->children = GROW_ARRAY(EngineNode*, node->children, oldCapacity, node->capacity);
	}

	//prune tombstones (experimental)
	int counter = 0;
	for (int i = 0; i < node->capacity; i++) {
		if (i >= node->count) {
			node->count = counter;
			break;
		}

		//move down
		if (node->children[i] != NULL) {
			node->children[counter++] = node->children[i];
		}
	}

	//assign
	node->children[node->count++] = child;

	//reverse-assign
	child->parent = node;
}

void freeEngineNode(EngineNode* node) {
	if (node == NULL) {
		return; //NO-OP
	}

	//free and tombstone this node
	for (int i = 0; i < node->count; i++) {
		freeEngineNode(node->children[i]);
	}

	//free the pointer array to the children
	FREE_ARRAY(EngineNode*, node->children, node->capacity);

	if (node->functions != NULL) {
		freeLiteralDictionary(node->functions);
		FREE(LiteralDictionary, node->functions);
	}

	//free this node's memory
	node->freeMemory(node);
}

Literal callEngineNodeLiteral(EngineNode* node, Interpreter* interpreter, Literal key, LiteralArray* args) {
	Literal ret = TO_NULL_LITERAL;

	//if this fn exists
	if (existsLiteralDictionary(node->functions, key)) {
		Literal fn = getLiteralDictionary(node->functions, key);
		Literal n = TO_OPAQUE_LITERAL(node, node->tag);

		LiteralArray arguments;
		LiteralArray returns;
		initLiteralArray(&arguments);
		initLiteralArray(&returns);

		//feed the arguments in backwards!
		if (args) {
			for (int i = args->count -1; i >= 0; i--) {
				pushLiteralArray(&arguments, args->literals[i]);
			}
		}

		pushLiteralArray(&arguments, n);

		callLiteralFn(interpreter, fn, &arguments, &returns);

		ret = popLiteralArray(&returns);

		freeLiteralArray(&arguments);
		freeLiteralArray(&returns);

		freeLiteral(n);
		freeLiteral(fn);
	}

	return ret;
}

Literal callEngineNode(EngineNode* node, Interpreter* interpreter, char* fnName, LiteralArray* args) {
	//call "fnName" on this node, and all children, if it exists
	Literal key = TO_IDENTIFIER_LITERAL(copyString(fnName, strlen(fnName)), strlen(fnName));

	Literal ret = callEngineNodeLiteral(node, interpreter, key, args);

	freeLiteral(key);

	return ret;
}

void callRecursiveEngineNodeLiteral(EngineNode* node, Interpreter* interpreter, Literal key, LiteralArray* args) {
	//if this fn exists
	if (existsLiteralDictionary(node->functions, key)) {
		Literal fn = getLiteralDictionary(node->functions, key);
		Literal n = TO_OPAQUE_LITERAL(node, node->tag);

		LiteralArray arguments;
		LiteralArray returns;
		initLiteralArray(&arguments);
		initLiteralArray(&returns);

		//feed the arguments in backwards!
		if (args) {
			for (int i = args->count -1; i >= 0; i--) {
				pushLiteralArray(&arguments, args->literals[i]);
			}
		}

		pushLiteralArray(&arguments, n);

		callLiteralFn(interpreter, fn, &arguments, &returns);

		freeLiteralArray(&arguments);
		freeLiteralArray(&returns);

		freeLiteral(n);
		freeLiteral(fn);
	}

	//recurse to the (non-tombstone) children
	for (int i = 0; i < node->count; i++) {
		if (node->children[i] != NULL) {
			callRecursiveEngineNodeLiteral(node->children[i], interpreter, key, args);
		}
	}
}

void callRecursiveEngineNode(EngineNode* node, Interpreter* interpreter, char* fnName, LiteralArray* args) {
	//call "fnName" on this node, and all children, if it exists
	Literal key = TO_IDENTIFIER_LITERAL(copyString(fnName, strlen(fnName)), strlen(fnName));

	callRecursiveEngineNodeLiteral(node, interpreter, key, args);

	freeLiteral(key);
}

int loadTextureEngineNode(EngineNode* node, char* fname) {
	SDL_Surface* surface = IMG_Load(fname);

	if (surface == NULL) {
		return -1;
	}

	node->texture = SDL_CreateTextureFromSurface(engine.renderer, surface);

	if (node->texture == NULL) {
		return -2;
	}

	SDL_FreeSurface(surface);

	int w, h;
	SDL_QueryTexture(node->texture, NULL, NULL, &w, &h);
	SDL_Rect r = { 0, 0, w, h };
	setRectEngineNode(node, r);

	return 0;
}

void freeTextureEngineNode(EngineNode* node) {
	if (node->texture != NULL) {
		SDL_DestroyTexture(node->texture);
		node->texture = NULL;
	}
}

void setRectEngineNode(EngineNode* node, SDL_Rect rect) {
	node->rect = rect;
}

void drawEngineNode(EngineNode* node, SDL_Rect dest) {
	SDL_RenderCopy(engine.renderer, node->texture, &node->rect, &dest);
}
