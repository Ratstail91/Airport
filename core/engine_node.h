#pragma once

#include "common.h"

#include "literal_dictionary.h"
#include "interpreter.h"

#define OPAQUE_TAG_ENGINE_NODE 1

//forward declare
typedef struct _engineNode EngineNode;
typedef void (*EngineNodeCallback)(void*);

//the node object, which forms a tree
typedef struct _engineNode {
	//function for releasing memory
	EngineNodeCallback freeMemory;

	//toy functions, stored in a dict for flexibility
	LiteralDictionary* functions;

	//point to the parent
	EngineNode* parent;

	//my opaque type tag
	int tag;
	int _unused;

	//use Toy's memory model
	EngineNode** children;
	int capacity;
	int count; //includes tombstones
} EngineNode;

CORE_API void initEngineNode(EngineNode* node, Interpreter* interpreter, void* tb, size_t size); //run bytecode, then grab all top-level function literals
CORE_API void pushEngineNode(EngineNode* node, EngineNode* child); //push to the array (prune tombstones when expanding/copying)
CORE_API void freeEngineNode(EngineNode* node); //free and tombstone this node

CORE_API void callEngineNode(EngineNode* node, Interpreter* interpreter, char* fnName); //call "fnName" on this node, and all children, if it exists
