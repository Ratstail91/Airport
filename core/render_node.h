#pragma once

#include "common.h"

//mimic the engine node
#include "engine_node.h"

#include <SDL2/SDL.h>

typedef struct _renderNode {
	//function for releasing memory
	EngineNodeCallback freeMemory;

	//toy functions, stored in a dict for flexibility
	LiteralDictionary* functions;

	//use Toy's memory model
	EngineNode** children;
	int capacity;
	int count; //includes tombstones

	//RenderNode-specific features
	SDL_Texture* texture;
	SDL_Rect rect;
	//TODO: depth
} RenderNode;

CORE_API void initRenderNode(RenderNode* node, Interpreter* interpreter, void* tb, size_t size);
CORE_API int loadTextureRenderNode(RenderNode* node, char* fname);
CORE_API void freeTextureRenderNode(RenderNode* node);

CORE_API void setRectRenderNode(RenderNode* node, SDL_Rect rect);
//TODO: getRectRenderNode
CORE_API void drawRenderNode(RenderNode* node, SDL_Rect dest);
