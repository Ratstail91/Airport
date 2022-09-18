#pragma once

#include "common.h"
#include "engine_node.h"

#include <SDL2/SDL.h>

#include <sys/time.h>

//the base engine object, which represents the state of the game
typedef struct _engine {
	//engine stuff
	EngineNode* root;
	struct timeval simTime;
	struct timeval realTime;
	bool running;

	//SDL stuff
	SDL_Window* window;
	SDL_Renderer* renderer;
	int screenWidth;
	int screenHeight;
} Engine;

//APIs for initializing the engine
CORE_API void initEngine(Engine* engine);
CORE_API void freeEngine(Engine* engine);

CORE_API void execEngine(Engine* engine);

