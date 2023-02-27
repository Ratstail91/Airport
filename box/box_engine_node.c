#include "box_engine_node.h"
#include "box_engine.h"

#include "toy_memory.h"

void Box_initEngineNode(Box_EngineNode* node, Toy_Interpreter* interpreter, const unsigned char* tb, size_t size) {
	//init
	// node->freeMemory = freeMemory;
	node->functions = TOY_ALLOCATE(Toy_LiteralDictionary, 1);
	node->parent = NULL;
	node->tag = OPAQUE_TAG_ENGINE_NODE;
	node->children = NULL;
	node->capacity = 0;
	node->count = 0;
	node->childCount = 0;
	node->texture = NULL;
	node->rect = ((SDL_Rect) { 0, 0, 0, 0 });
	node->frames = 0;

	Toy_initLiteralDictionary(node->functions);

	//run bytecode
	Toy_runInterpreter(interpreter, tb, size);

	//grab all top-level functions from the dirty interpreter
	Toy_LiteralDictionary* variablesPtr = &interpreter->scope->variables;

	for (int i = 0; i < variablesPtr->capacity; i++) {
		//skip empties and tombstones
		if (TOY_IS_NULL(variablesPtr->entries[i].key)) {
			continue;
		}

		//if this variable is a function (this outmodes import and export)
		Toy_private_dictionary_entry* entry = &variablesPtr->entries[i];
		if (TOY_IS_FUNCTION(entry->value)) {
			//save a copy
			Toy_setLiteralDictionary(node->functions, entry->key, entry->value);
		}
	}
}

void Box_pushEngineNode(Box_EngineNode* node, Box_EngineNode* child) {
	//push to the array
	if (node->count + 1 > node->capacity) {
		int oldCapacity = node->capacity;

		node->capacity = TOY_GROW_CAPACITY(oldCapacity);
		node->children = TOY_GROW_ARRAY(Box_EngineNode*, node->children, oldCapacity, node->capacity);
	}

	//assign
	node->children[node->count++] = child;

	//reverse-assign
	child->parent = node;

	//count
	node->childCount++;
}

void Box_freeEngineNode(Box_EngineNode* node) {
	if (node == NULL) {
		return; //NO-OP
	}

	//free and tombstone this node
	for (int i = 0; i < node->count; i++) {
		Box_freeEngineNode(node->children[i]);
	}

	//free the pointer array to the children
	TOY_FREE_ARRAY(Box_EngineNode*, node->children, node->capacity);

	if (node->functions != NULL) {
		Toy_freeLiteralDictionary(node->functions);
		TOY_FREE(Toy_LiteralDictionary, node->functions);
	}

	if (node->texture != NULL) {
		Box_freeTextureEngineNode(node);
	}

	//free this node's memory
	TOY_FREE(Box_EngineNode, node);
}

Box_EngineNode* Box_getChildEngineNode(Box_EngineNode* node, int index) {
	if (index < 0 || index > node->count) {
		return NULL;
	}

	return node->children[index];
}

void Box_freeChildEngineNode(Box_EngineNode* node, int index) {
	//get the child node
	Box_EngineNode* childNode = node->children[index];

	//free the node
	if (childNode != NULL) {
		Box_callRecursiveEngineNode(childNode, &engine.interpreter, "onFree", NULL);
		Box_freeEngineNode(childNode);
		node->childCount--;
	}

	node->children[index] = NULL;
}

Toy_Literal Box_callEngineNodeLiteral(Box_EngineNode* node, Toy_Interpreter* interpreter, Toy_Literal key, Toy_LiteralArray* args) {
	Toy_Literal ret = TOY_TO_NULL_LITERAL;

	//if this fn exists
	if (Toy_existsLiteralDictionary(node->functions, key)) {
		Toy_Literal fn = Toy_getLiteralDictionary(node->functions, key);
		Toy_Literal n = TOY_TO_OPAQUE_LITERAL(node, node->tag);

		Toy_LiteralArray arguments;
		Toy_LiteralArray returns;
		Toy_initLiteralArray(&arguments);
		Toy_initLiteralArray(&returns);

		//feed the arguments in
		Toy_pushLiteralArray(&arguments, n);

		if (args) {
			for (int i = 0; i < args->count; i++) {
				Toy_pushLiteralArray(&arguments, args->literals[i]);
			}
		}

		Toy_callLiteralFn(interpreter, fn, &arguments, &returns);

		ret = Toy_popLiteralArray(&returns);

		Toy_freeLiteralArray(&arguments);
		Toy_freeLiteralArray(&returns);

		Toy_freeLiteral(n);
		Toy_freeLiteral(fn);
	}

	return ret;
}

Toy_Literal Box_callEngineNode(Box_EngineNode* node, Toy_Interpreter* interpreter, const char* fnName, Toy_LiteralArray* args) {
	//call "fnName" on this node, and all children, if it exists
	Toy_Literal key = TOY_TO_IDENTIFIER_LITERAL(Toy_createRefString(fnName));

	Toy_Literal ret = Box_callEngineNodeLiteral(node, interpreter, key, args);

	Toy_freeLiteral(key);

	return ret;
}

void Box_callRecursiveEngineNodeLiteral(Box_EngineNode* node, Toy_Interpreter* interpreter, Toy_Literal key, Toy_LiteralArray* args) {
	//if this fn exists
	if (Toy_existsLiteralDictionary(node->functions, key)) {
		Toy_Literal fn = Toy_getLiteralDictionary(node->functions, key);
		Toy_Literal n = TOY_TO_OPAQUE_LITERAL(node, node->tag);

		Toy_LiteralArray arguments;
		Toy_LiteralArray returns;
		Toy_initLiteralArray(&arguments);
		Toy_initLiteralArray(&returns);

		//feed the arguments in
		Toy_pushLiteralArray(&arguments, n);

		if (args) {
			for (int i = 0; i < args->count; i++) {
				Toy_pushLiteralArray(&arguments, args->literals[i]);
			}
		}

		Toy_callLiteralFn(interpreter, fn, &arguments, &returns);

		Toy_freeLiteralArray(&arguments);
		Toy_freeLiteralArray(&returns);

		Toy_freeLiteral(n);
		Toy_freeLiteral(fn);
	}

	//recurse to the (non-tombstone) children
	for (int i = 0; i < node->count; i++) {
		if (node->children[i] != NULL) {
			Box_callRecursiveEngineNodeLiteral(node->children[i], interpreter, key, args);
		}
	}
}

void Box_callRecursiveEngineNode(Box_EngineNode* node, Toy_Interpreter* interpreter, const char* fnName, Toy_LiteralArray* args) {
	//call "fnName" on this node, and all children, if it exists
	Toy_Literal key = TOY_TO_IDENTIFIER_LITERAL(Toy_createRefString(fnName));

	Box_callRecursiveEngineNodeLiteral(node, interpreter, key, args);

	Toy_freeLiteral(key);
}

int Box_getChildCountEngineNode(Box_EngineNode* node) {
	return node->childCount;
}

int Box_loadTextureEngineNode(Box_EngineNode* node, const char* fname) {
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
	Box_setRectEngineNode(node, r);
	Box_setFramesEngineNode(node, 1); //default

	return 0;
}

void Box_freeTextureEngineNode(Box_EngineNode* node) {
	if (node->texture != NULL) {
		SDL_DestroyTexture(node->texture);
		node->texture = NULL;
	}
}

void Box_setRectEngineNode(Box_EngineNode* node, SDL_Rect rect) {
	node->rect = rect;
}

SDL_Rect Box_getRectEngineNode(Box_EngineNode* node) {
	return node->rect;
}

void Box_setFramesEngineNode(Box_EngineNode* node, int frames) {
	node->frames = frames;
	node->currentFrame = 0; //just in case
}

int Box_getFramesEngineNode(Box_EngineNode* node) {
	return node->frames;
}

void Box_setCurrentFrameEngineNode(Box_EngineNode* node, int currentFrame) {
	node->currentFrame = currentFrame;
}

int Box_getCurrentFrameEngineNode(Box_EngineNode* node) {
	return node->currentFrame;
}

void Box_incrementCurrentFrame(Box_EngineNode* node) {
	node->currentFrame++;
	if (node->currentFrame >= node->frames) {
		node->currentFrame = 0;
	}
}

void Box_drawEngineNode(Box_EngineNode* node, SDL_Rect dest) {
	SDL_Rect src = node->rect;
	src.x += src.w * node->currentFrame; //TODO: improve this
	SDL_RenderCopy(engine.renderer, node->texture, &src, &dest);
}
