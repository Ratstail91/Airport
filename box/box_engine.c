#include "box_engine.h"

#include "lib_engine.h"
#include "lib_input.h"
#include "lib_node.h"
#include "lib_standard.h"
#include "lib_timer.h"
#include "lib_runner.h"
#include "repl_tools.h"

#include "toy_memory.h"
#include "toy_lexer.h"
#include "toy_parser.h"
#include "toy_compiler.h"
#include "toy_interpreter.h"
#include "toy_literal_array.h"
#include "toy_literal_dictionary.h"

#include "toy_console_colors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//define the extern engine object
Box_Engine engine;

//errors here should be fatal
static void fatalError(char* message) {
	fprintf(stderr, TOY_CC_ERROR "%s" TOY_CC_RESET, message);
	exit(-1);
}

//exposed functions
void Box_initEngine() {
	//clear
	engine.rootNode = NULL;
	engine.running = false;
	engine.window = NULL;
	engine.renderer = NULL;

	//init SDL
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fatalError("Failed to initialize SDL2");
	}

	//init events
	Toy_initLiteralArray(&engine.keyDownEvents);
	Toy_initLiteralDictionary(&engine.symKeyDownEvents);
	Toy_initLiteralArray(&engine.keyUpEvents);
	Toy_initLiteralDictionary(&engine.symKeyUpEvents);

	//init Toy
	Toy_initInterpreter(&engine.interpreter);
	Toy_injectNativeHook(&engine.interpreter, "engine", Box_hookEngine);
	Toy_injectNativeHook(&engine.interpreter, "node", Box_hookNode);
	Toy_injectNativeHook(&engine.interpreter, "input", Box_hookInput);
	Toy_injectNativeHook(&engine.interpreter, "standard", Toy_hookStandard);
	Toy_injectNativeHook(&engine.interpreter, "timer", Toy_hookTimer);
	Toy_injectNativeHook(&engine.interpreter, "runner", Toy_hookRunner);

	size_t size = 0;
	char* source = Toy_readFile("./assets/scripts/init.toy", &size);
	unsigned char* tb = Toy_compileString(source, &size);
	free((void*)source);

	Toy_runInterpreter(&engine.interpreter, tb, size);
}

void Box_freeEngine() {
	//clear existing root node
	if (engine.rootNode != NULL) {
		Box_callRecursiveEngineNode(engine.rootNode, &engine.interpreter, "onFree", NULL);

		Box_freeEngineNode(engine.rootNode);

		engine.rootNode = NULL;
	}

	Toy_freeInterpreter(&engine.interpreter);

	//free events
	Toy_freeLiteralArray(&engine.keyDownEvents);
	Toy_freeLiteralDictionary(&engine.symKeyDownEvents);
	Toy_freeLiteralArray(&engine.keyUpEvents);
	Toy_freeLiteralDictionary(&engine.symKeyUpEvents);

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
		Toy_freeLiteralArray(&engine.keyDownEvents);
		//NOTE: this is likely memory intensive - a more bespoke linked list designed for this task would be better
		//NOTE: alternatively - manual memory-wipes, skipping the free step could be better
	}

	if (engine.keyUpEvents.count > 0) {
		Toy_freeLiteralArray(&engine.keyUpEvents);
	}

	//poll all events
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
				//bugfix: ignore repeat messages
				if (event.key.repeat) {
					break;
				}

				//determine the given keycode
				Toy_Literal keycodeLiteral = TOY_TO_INTEGER_LITERAL( (int)(event.key.keysym.sym) );
				if (!Toy_existsLiteralDictionary(&engine.symKeyDownEvents, keycodeLiteral)) {
					break;
				}

				//get the event name
				Toy_Literal eventLiteral = Toy_getLiteralDictionary(&engine.symKeyDownEvents, keycodeLiteral);

				//push to the event list
				Toy_pushLiteralArray(&engine.keyDownEvents, eventLiteral);
			}
			break;

			case SDL_KEYUP: {
				//bugfix: ignore repeat messages
				if (event.key.repeat) {
					break;
				}

				//determine the given keycode
				Toy_Literal keycodeLiteral = TOY_TO_INTEGER_LITERAL( (int)(event.key.keysym.sym) );
				if (!Toy_existsLiteralDictionary(&engine.symKeyUpEvents, keycodeLiteral)) {
					break;
				}

				//get the event name
				Toy_Literal eventLiteral = Toy_getLiteralDictionary(&engine.symKeyUpEvents, keycodeLiteral);

				//push to the event list
				Toy_pushLiteralArray(&engine.keyUpEvents, eventLiteral);
			}
			break;
		}
	}

	//process input events
	if (engine.rootNode != NULL) {
		//key down events
		for (int i = 0; i < engine.keyDownEvents.count; i++) { //TODO: could pass in the whole array?
			Toy_LiteralArray args;
			Toy_initLiteralArray(&args);
			Toy_pushLiteralArray(&args, engine.keyDownEvents.literals[i]);
			Box_callRecursiveEngineNode(engine.rootNode, &engine.interpreter, "onKeyDown", &args);
			Toy_freeLiteralArray(&args);
		}

		//key up events
		for (int i = 0; i < engine.keyUpEvents.count; i++) {
			Toy_LiteralArray args;
			Toy_initLiteralArray(&args);
			Toy_pushLiteralArray(&args, engine.keyUpEvents.literals[i]);
			Box_callRecursiveEngineNode(engine.rootNode, &engine.interpreter, "onKeyUp", &args);
			Toy_freeLiteralArray(&args);
		}
	}
}

void execStep() {
	if (engine.rootNode != NULL) {
		//steps
		Box_callRecursiveEngineNode(engine.rootNode, &engine.interpreter, "onStep", NULL);
	}
}

//the heart of the engine
void Box_execEngine() {
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

		Box_callRecursiveEngineNode(engine.rootNode, &engine.interpreter, "onDraw", NULL);

		SDL_RenderPresent(engine.renderer);
	}
}
