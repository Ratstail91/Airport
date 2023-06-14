#pragma once

#include "box_common.h"

#include "toy_literal_dictionary.h"
#include "toy_interpreter.h"

#define OPAQUE_TAG_NODE 1001

//forward declare
typedef struct Box_private_node Box_Node;
// typedef void (*Box_NodeCallback)(void*);

//the node object, which forms a tree
typedef struct Box_private_node {
	//function for releasing memory NOTE: removed, because it's not needed with only 1 node type - I've left them commented out because I might need them soon
	// Box_NodeCallback freeMemory;

	//toy functions, stored in a dict for flexibility
	Toy_LiteralDictionary* functions;

	//point to the parent
	Box_Node* parent;

	//BUGFIX: hold the node's scope so it can be popped
	Toy_Scope* scope;

	//my opaque type tag
	int tag;

	//use Toy's memory model
	Box_Node** children;
	int capacity;
	int count; //includes tombstones
	int childCount;

	//rendering-specific features
	SDL_Texture* texture;
	SDL_Rect rect;
	int frames;
	int currentFrame;
} Box_Node; //TODO: rename this?

BOX_API void Box_initNode(Box_Node* node, Toy_Interpreter* interpreter, const unsigned char* tb, size_t size); //run bytecode, then grab all top-level function literals
BOX_API void Box_pushNode(Box_Node* node, Box_Node* child); //push to the array (prune tombstones when expanding/copying)
BOX_API void Box_freeNode(Box_Node* node); //free this node and all children

BOX_API Box_Node* Box_getChildNode(Box_Node* node, int index);
BOX_API void Box_freeChildNode(Box_Node* node, int index);

BOX_API Toy_Literal Box_callNodeLiteral(Box_Node* node, Toy_Interpreter* interpreter, Toy_Literal key, Toy_LiteralArray* args);
BOX_API Toy_Literal Box_callNode(Box_Node* node, Toy_Interpreter* interpreter, const char* fnName, Toy_LiteralArray* args); //call "fnName" on this node, and only this node, if it exists

//for calling various lifecycle functions
BOX_API void Box_callRecursiveNodeLiteral(Box_Node* node, Toy_Interpreter* interpreter, Toy_Literal key, Toy_LiteralArray* args);
BOX_API void Box_callRecursiveNode(Box_Node* node, Toy_Interpreter* interpreter, const char* fnName, Toy_LiteralArray* args); //call "fnName" on this node, and all children, if it exists

BOX_API int Box_getChildCountNode(Box_Node* node);

BOX_API int Box_loadTextureNode(Box_Node* node, const char* fname);
BOX_API void Box_freeTextureNode(Box_Node* node);

BOX_API void Box_setRectNode(Box_Node* node, SDL_Rect rect);
BOX_API SDL_Rect Box_getRectNode(Box_Node* node);

BOX_API void Box_setFramesNode(Box_Node* node, int frames);
BOX_API int Box_getFramesNode(Box_Node* node);
BOX_API void Box_setCurrentFrameNode(Box_Node* node, int currentFrame);
BOX_API int Box_getCurrentFrameNode(Box_Node* node);
BOX_API void Box_incrementCurrentFrame(Box_Node* node);

BOX_API void Box_setTextNode(Box_Node* node, TTF_Font* font, const char* text, SDL_Color color);

BOX_API void Box_drawNode(Box_Node* node, SDL_Rect dest);
