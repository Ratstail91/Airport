#pragma once

#include "core_common.h"
#include "engine_node.h"
#include "interpreter.h"

#include "literal_array.h"
#include "literal_dictionary.h"

#include "core_common.h"

#include <sys/time.h>

//the base engine object, which represents the state of the game
typedef struct _engine {
	//engine stuff
	EngineNode* rootNode;
	struct timeval simTime;
	struct timeval realTime;
	bool running;

	//Toy stuff
	Interpreter interpreter;

	//SDL stuff
	SDL_Window* window;
	SDL_Renderer* renderer;
	int screenWidth;
	int screenHeight;

	//input syms mapped to events
	LiteralArray keyDownEvents; //list of events that occurred this frame
	LiteralDictionary symKeyDownEvents; //keysym -> event names

	LiteralArray keyUpEvents; //list of events that occurred this frame
	LiteralDictionary symKeyUpEvents; //keysym -> event names
} Engine;

//extern singleton
extern Engine engine;

//APIs for running the engine in main()
CORE_API void initEngine();
CORE_API void execEngine();
CORE_API void freeEngine();

