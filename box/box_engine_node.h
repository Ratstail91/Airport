#pragma once

#include "box_common.h"

#include "toy_literal_dictionary.h"
#include "toy_interpreter.h"

#define OPAQUE_TAG_ENGINE_NODE 1001

//forward declare
typedef struct Box_private_engineNode Box_EngineNode;
// typedef void (*EngineNodeCallback)(void*);

//the node object, which forms a tree
typedef struct Box_private_engineNode {
	//function for releasing memory NOTE: removed, because it's not needed with only 1 node type - I've left them commented out because I might need them soon
	// EngineNodeCallback freeMemory;

	//toy functions, stored in a dict for flexibility
	Toy_LiteralDictionary* functions;

	//point to the parent
	Box_EngineNode* parent;

	//my opaque type tag
	int tag;
	// int _unused0;

	//use Toy's memory model
	Box_EngineNode** children;
	int capacity;
	int count; //includes tombstones

	//rendering-specific features
	SDL_Texture* texture;
	SDL_Rect rect;
	//TODO: depth
} Box_EngineNode;

BOX_API void Box_initEngineNode(Box_EngineNode* node, Toy_Interpreter* interpreter, const unsigned char* tb, size_t size); //run bytecode, then grab all top-level function literals
BOX_API void Box_pushEngineNode(Box_EngineNode* node, Box_EngineNode* child); //push to the array (prune tombstones when expanding/copying)
BOX_API void Box_freeEngineNode(Box_EngineNode* node); //free and tombstone this node

BOX_API Toy_Literal Box_callEngineNodeLiteral(Box_EngineNode* node, Toy_Interpreter* interpreter, Toy_Literal key, Toy_LiteralArray* args);
BOX_API Toy_Literal Box_callEngineNode(Box_EngineNode* node, Toy_Interpreter* interpreter, const char* fnName, Toy_LiteralArray* args); //call "fnName" on this node, and only this node, if it exists

BOX_API void Box_callRecursiveEngineNodeLiteral(Box_EngineNode* node, Toy_Interpreter* interpreter, Toy_Literal key, Toy_LiteralArray* args);
BOX_API void Box_callRecursiveEngineNode(Box_EngineNode* node, Toy_Interpreter* interpreter, const char* fnName, Toy_LiteralArray* args); //call "fnName" on this node, and all children, if it exists

BOX_API int Box_loadTextureEngineNode(Box_EngineNode* node, const char* fname);
BOX_API void Box_freeTextureEngineNode(Box_EngineNode* node);

BOX_API void Box_setRectEngineNode(Box_EngineNode* node, SDL_Rect rect);
//TODO: getRect
BOX_API void Box_drawEngineNode(Box_EngineNode* node, SDL_Rect dest);
