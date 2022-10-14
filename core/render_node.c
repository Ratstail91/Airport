#include "render_node.h"

#include "engine.h"
#include "memory.h"

#include <SDL2/SDL_image.h>

static void freeMemory(void* ptr) {
	RenderNode* node = (RenderNode*)ptr;

	SDL_DestroyTexture(node->texture);

	//free this node type's memory
	FREE(RenderNode, ptr);
}

//duplicate the initEngineNode() functionality, plus extra stuff
void initRenderNode(RenderNode* node, Interpreter* interpreter, void* tb, size_t size) {
	//init
	node->freeMemory = freeMemory;
	node->functions = ALLOCATE(LiteralDictionary, 1);
	node->parent = NULL;
	node->tag = OPAQUE_TAG_RENDER_NODE;
	node->children = NULL;
	node->capacity = 0;
	node->count = 0;

	node->texture = NULL;

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

int loadTextureRenderNode(RenderNode* node, char* fname) {
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
	setRectRenderNode(node, r);

	return 0;
}

void freeTextureRenderNode(RenderNode* node) {
	if (node->texture != NULL) {
		SDL_DestroyTexture(node->texture);
		node->texture = NULL;
	}
}

void setRectRenderNode(RenderNode* node, SDL_Rect rect) {
	node->rect = rect;
}

void drawRenderNode(RenderNode* node, SDL_Rect dest) {
	SDL_RenderCopy(engine.renderer, node->texture, &node->rect, &dest);
}
