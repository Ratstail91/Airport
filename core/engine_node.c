#include "engine_node.h"

#include "memory.h"

STATIC_ASSERT(sizeof(EngineNode) == 32);
STATIC_ASSERT(sizeof(EngineNodeCallback) == 8);
STATIC_ASSERT(sizeof(LiteralDictionary*) == 8);
STATIC_ASSERT(sizeof(EngineNode*) == 8);
STATIC_ASSERT(sizeof(int) == 4);

static void freeMemory(void* ptr) {
	//free this node type's memory
	FREE(EngineNode, ptr);
}

void initEngineNode(EngineNode* node, Interpreter* interpreter, void* tb, size_t size) {
	//init
	node->freeMemory = freeMemory;
	node->functions = ALLOCATE(LiteralDictionary, 1);
	node->children = NULL;
	node->capacity = 0;
	node->count = 0;

	initLiteralDictionary(node->functions);

	//run bytecode
	runInterpreter(interpreter, tb, size);

	//grab all top-level function literals
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

static void callEngineNodeLiteral(EngineNode* node, Interpreter* interpreter, Literal key) {
	//if this fn exists
	if (existsLiteralDictionary(node->functions, key)) {
		Literal fn = getLiteralDictionary(node->functions, key);
		Literal n = TO_OPAQUE_LITERAL(node, -1);

		LiteralArray arguments;
		LiteralArray returns;
		initLiteralArray(&arguments);
		initLiteralArray(&returns);

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
			callEngineNodeLiteral(node->children[i], interpreter, key);
		}
	}
}

void callEngineNode(EngineNode* node, Interpreter* interpreter, char* fnName) {
	//call "fnName" on this node, and all children, if it exists
	Literal key = TO_IDENTIFIER_LITERAL(copyString(fnName, strlen(fnName)), strlen(fnName));

	callEngineNodeLiteral(node, interpreter, key);

	freeLiteral(key);
}
