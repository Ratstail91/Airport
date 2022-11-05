#include "engine.h"

#include "lib_engine.h"
#include "lib_input.h"
#include "lib_node.h"
#include "lib_standard.h"
#include "repl_tools.h"

#include "memory.h"
#include "lexer.h"
#include "parser.h"
#include "compiler.h"
#include "interpreter.h"
#include "literal_array.h"
#include "literal_dictionary.h"

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

	//init events
	initLiteralArray(&engine.keyDownEvents);
	initLiteralDictionary(&engine.symKeyDownEvents);
	initLiteralArray(&engine.keyUpEvents);
	initLiteralDictionary(&engine.symKeyUpEvents);

	//init Toy
	initInterpreter(&engine.interpreter);
	injectNativeHook(&engine.interpreter, "engine", hookEngine);
	injectNativeHook(&engine.interpreter, "node", hookNode);
	injectNativeHook(&engine.interpreter, "input", hookInput);
	injectNativeHook(&engine.interpreter, "standard", hookStandard);

	size_t size = 0;
	char* source = readFile("./assets/scripts/init.toy", &size);
	unsigned char* tb = compileString(source, &size);
	free((void*)source);

	runInterpreter(&engine.interpreter, tb, size);
}

void freeEngine() {
	//clear existing root node
	if (engine.rootNode != NULL) {
		callRecursiveEngineNode(engine.rootNode, &engine.interpreter, "onFree", NULL);

		freeEngineNode(engine.rootNode);

		engine.rootNode = NULL;
	}

	freeInterpreter(&engine.interpreter);

	//free events
	freeLiteralArray(&engine.keyDownEvents);
	freeLiteralDictionary(&engine.symKeyDownEvents);
	freeLiteralArray(&engine.keyUpEvents);
	freeLiteralDictionary(&engine.symKeyUpEvents);

	//free SDL
	SDL_DestroyRenderer(engine.renderer);
	SDL_DestroyWindow(engine.window);
	SDL_Quit();

	engine.renderer = NULL;
	engine.window = NULL;
}

static void execEvents() {
	//clear event lists
	if (engine.keyDownEvents.count > 0) {
		freeLiteralArray(&engine.keyDownEvents);
		//NOTE: this is likely memory intensive - a more bespoke linked list designed for this task would be better
		//NOTE: alternatively - manual memory-wipes, skipping the free step could be better
	}

	if (engine.keyUpEvents.count > 0) {
		freeLiteralArray(&engine.keyUpEvents);
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

			//input
			case SDL_KEYDOWN: {
				//determine the given keycode
				Literal keycodeLiteral = TO_INTEGER_LITERAL( (int)(event.key.keysym.sym) );
				if (!existsLiteralDictionary(&engine.symKeyDownEvents, keycodeLiteral)) {
					break;
				}

				//get the event name
				Literal eventLiteral = getLiteralDictionary(&engine.symKeyDownEvents, keycodeLiteral);

				//push to the event list
				pushLiteralArray(&engine.keyDownEvents, eventLiteral);
			}
			break;

			case SDL_KEYUP: {
				//determine the given keycode
				Literal keycodeLiteral = TO_INTEGER_LITERAL( (int)(event.key.keysym.sym) );
				if (!existsLiteralDictionary(&engine.symKeyUpEvents, keycodeLiteral)) {
					break;
				}

				//get the event name
				Literal eventLiteral = getLiteralDictionary(&engine.symKeyUpEvents, keycodeLiteral);

				//push to the event list
				pushLiteralArray(&engine.keyUpEvents, eventLiteral);
			}
			break;
		}
	}

	//callbacks
	if (engine.rootNode != NULL) {
		//key down events
		for (int i = 0; i < engine.keyDownEvents.count; i++) {
			LiteralArray args;
			initLiteralArray(&args);
			pushLiteralArray(&args, engine.keyDownEvents.literals[i]);
			callRecursiveEngineNode(engine.rootNode, &engine.interpreter, "onKeyDown", &args);
			freeLiteralArray(&args);
		}

		//key up events
		for (int i = 0; i < engine.keyUpEvents.count; i++) {
			LiteralArray args;
			initLiteralArray(&args);
			pushLiteralArray(&args, engine.keyUpEvents.literals[i]);
			callRecursiveEngineNode(engine.rootNode, &engine.interpreter, "onKeyUp", &args);
			freeLiteralArray(&args);
		}
	}
}

void execStep() {
	if (engine.rootNode != NULL) {
		//steps
		callRecursiveEngineNode(engine.rootNode, &engine.interpreter, "onStep", NULL);
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
		execEvents();

		//calc the time passed
		gettimeofday(&engine.realTime, NULL);

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

		callRecursiveEngineNode(engine.rootNode, &engine.interpreter, "onDraw", NULL);

		SDL_RenderPresent(engine.renderer);
	}
}
