#pragma once

#include "core_common.h"

#include "literal_dictionary.h"
#include "interpreter.h"

#define OPAQUE_TAG_ENGINE_NODE 1001

//forward declare
typedef struct _engineNode EngineNode;
// typedef void (*EngineNodeCallback)(void*);

//the node object, which forms a tree
typedef struct _engineNode {
	//function for releasing memory NOTE: removed, because it's not needed with only 1 node type - I've left them commented out because I might need them soon
	// EngineNodeCallback freeMemory;

	//toy functions, stored in a dict for flexibility
	LiteralDictionary* functions;

	//point to the parent
	EngineNode* parent;

	//my opaque type tag
	int tag;
	int _unused0;

	//use Toy's memory model
	EngineNode** children;
	int capacity;
	int count; //includes tombstones

	//rendering-specific features
	SDL_Texture* texture;
	SDL_Rect rect;
	//TODO: depth
} EngineNode;

CORE_API void initEngineNode(EngineNode* node, Interpreter* interpreter, void* tb, size_t size); //run bytecode, then grab all top-level function literals
CORE_API void pushEngineNode(EngineNode* node, EngineNode* child); //push to the array (prune tombstones when expanding/copying)
CORE_API void freeEngineNode(EngineNode* node); //free and tombstone this node

CORE_API Literal callEngineNodeLiteral(EngineNode* node, Interpreter* interpreter, Literal key, LiteralArray* args);
CORE_API Literal callEngineNode(EngineNode* node, Interpreter* interpreter, char* fnName, LiteralArray* args); //call "fnName" on this node, and only this node, if it exists

CORE_API void callRecursiveEngineNodeLiteral(EngineNode* node, Interpreter* interpreter, Literal key, LiteralArray* args);
CORE_API void callRecursiveEngineNode(EngineNode* node, Interpreter* interpreter, char* fnName, LiteralArray* args); //call "fnName" on this node, and all children, if it exists

CORE_API int loadTextureEngineNode(EngineNode* node, char* fname);
CORE_API void freeTextureEngineNode(EngineNode* node);

CORE_API void setRectEngineNode(EngineNode* node, SDL_Rect rect);
//TODO: getRect
CORE_API void drawEngineNode(EngineNode* node, SDL_Rect dest);
