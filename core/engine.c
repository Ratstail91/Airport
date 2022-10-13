#include "engine.h"

#include "lib_engine.h"
#include "lib_render.h"
#include "lib_standard.h"
#include "repl_tools.h"

#include "memory.h"
#include "lexer.h"
#include "parser.h"
#include "compiler.h"
#include "interpreter.h"

#include "console_colors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//define the engine object
Engine engine;

//errors here should be fatal
static void fatalError(char* message) {
	fprintf(stderr, message);
	exit(-1);
}

//exposed functions
void initEngine() {
	//clear
	engine.rootNode = NULL;
	engine.running = false;
	engine.window = NULL;
	engine.renderer = NULL;

	//init SDL
	if (SDL_Init(0) != 0) {
		fatalError("Failed to initialize SDL2");
	}

	//init Toy
	initInterpreter(&engine.interpreter);
	injectNativeHook(&engine.interpreter, "engine", hookEngine);
	injectNativeHook(&engine.interpreter, "render", hookRender);
	injectNativeHook(&engine.interpreter, "standard", hookStandard);

	size_t size = 0;
	char* source = readFile("./assets/scripts/init.toy", &size);
	unsigned char* tb = compileString(source, &size);
	free((void*)source);

	runInterpreter(&engine.interpreter, tb, size);
}

void freeEngine() {
	SDL_DestroyRenderer(engine.renderer);
	SDL_DestroyWindow(engine.window);
	SDL_Quit();

	//clear existing root node
	if (engine.rootNode != NULL) {
		callEngineNode(engine.rootNode, &engine.interpreter, "onFree");

		freeEngineNode(engine.rootNode);

		engine.rootNode = NULL;
	}

	freeInterpreter(&engine.interpreter);

	engine.renderer = NULL;
	engine.window = NULL;
}

static void execStep() {
	//call onStep
	if (engine.rootNode != NULL) {
		callEngineNode(engine.rootNode, &engine.interpreter, "onStep");
	}

	//poll events
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch(event.type) {
			//quit
			case SDL_QUIT: {
				engine.running = false;
			}
			break;

			//window events are handled internally
			case SDL_WINDOWEVENT: {
				switch(event.window.event) {
					case SDL_WINDOWEVENT_RESIZED:
						engine.screenWidth = event.window.data1;
						engine.screenHeight = event.window.data2;
						SDL_RenderSetLogicalSize(engine.renderer, engine.screenWidth, engine.screenHeight);
					break;
				}
			}
			break;

			//TODO: input
		}
	}
}

//the heart of the engine
void execEngine() {
	if (!engine.running) {
		fatalError("Can't execute the engine (did you forget to initialize the screen?)");
	}

	//set up time
	gettimeofday(&engine.realTime, NULL);
	engine.simTime = engine.realTime;
	struct timeval delta = { .tv_sec = 0, .tv_usec = 1000 * 1000 / 60 }; //60 frames per second

	while (engine.running) {
		//calc the time passed
		gettimeofday(&engine.realTime, NULL);

		// printf("real time: %ld.%ld   sim time: %ld.%ld    + (delta: %ld.%ld)\n",
		// 	engine.realTime.tv_sec,
		// 	engine.realTime.tv_usec,
		// 	engine.simTime.tv_sec,
		// 	engine.simTime.tv_usec,
		// 	delta.tv_sec,
		// 	delta.tv_usec
		// );

		//if not enough time has passed
		if (timercmp(&engine.simTime, &engine.realTime, <)) {
			//while not enough time has passed
			while(timercmp(&engine.simTime, &engine.realTime, <)) {
				//simulate the world
				execStep();

				//calc the time simulation
				timeradd(&delta, &engine.simTime, &engine.simTime);
			}
		}
		else {
			SDL_Delay(10); //let the machine sleep, 10ms
		}

		//render the world
		SDL_SetRenderDrawColor(engine.renderer, 0, 0, 0, 255); //NOTE: This line can be disabled later
		SDL_RenderClear(engine.renderer); //NOTE: This line can be disabled later

		callEngineNode(engine.rootNode, &engine.interpreter, "onDraw");

		SDL_RenderPresent(engine.renderer);
	}
}
