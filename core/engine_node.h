#pragma once

#include "common.h"

//forward declare
typedef struct _engineNode EngineNode;
typedef struct _engine Engine;

//the interface function
typedef void (*EngineNodeFn)(EngineNode* self, Engine* engine);

//the node object, which forms a tree
typedef struct _engineNode {
	//use Toy's memory model
	void* children;
	int capacity;
	int count;

	EngineNodeFn onInit;
	EngineNodeFn onStep;
	EngineNodeFn onFree;
} EngineNode;

