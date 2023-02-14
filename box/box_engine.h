#pragma once

#include "box_common.h"
#include "box_engine_node.h"

#include "toy_interpreter.h"
#include "toy_literal_array.h"
#include "toy_literal_dictionary.h"

//TODO: remove this, and replace with time.h
#include <sys/time.h>

//the base engine object, which represents the state of the game
typedef struct Box_private_engine {
	//engine stuff
	Box_EngineNode* rootNode;
	struct timeval simTime;
	struct timeval realTime;
	bool running;

	//Toy stuff
	Toy_Interpreter interpreter;

	//SDL stuff
	SDL_Window* window;
	SDL_Renderer* renderer;
	int screenWidth;
	int screenHeight;

	//input syms mapped to events
	Toy_LiteralDictionary symKeyDownEvents; //keysym -> event names
	Toy_LiteralDictionary symKeyUpEvents; //keysym -> event names
} Box_Engine;

//extern singleton - used by various libraries
extern Box_Engine engine;

//APIs for running the engine in main()
BOX_API void Box_initEngine();
BOX_API void Box_execEngine();
BOX_API void Box_freeEngine();

