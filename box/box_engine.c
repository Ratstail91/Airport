#include "box_engine.h"

#include "lib_engine.h"
#include "lib_input.h"
#include "lib_node.h"
#include "lib_standard.h"
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
	Toy_initLiteralDictionary(&engine.symKeyDownEvents);
	Toy_initLiteralDictionary(&engine.symKeyUpEvents);

	//init Toy
	Toy_initInterpreter(&engine.interpreter);
	Toy_injectNativeHook(&engine.interpreter, "standard", Toy_hookStandard);
	Toy_injectNativeHook(&engine.interpreter, "runner", Toy_hookRunner);
	Toy_injectNativeHook(&engine.interpreter, "engine", Box_hookEngine);
	Toy_injectNativeHook(&engine.interpreter, "node", Box_hookNode);
	Toy_injectNativeHook(&engine.interpreter, "input", Box_hookInput);

	size_t size = 0;
	const char* source = Toy_readFile("./assets/scripts/init.toy", &size);
	const unsigned char* tb = Toy_compileString(source, &size);
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
	Toy_freeLiteralDictionary(&engine.symKeyDownEvents);
	Toy_freeLiteralDictionary(&engine.symKeyUpEvents);

	//free SDL
	SDL_DestroyRenderer(engine.renderer);
	SDL_DestroyWindow(engine.window);
	SDL_Quit();

	engine.renderer = NULL;
	engine.window = NULL;
}

static void execEvents() {
	Toy_LiteralArray args; //save some allocation by reusing this
	Toy_initLiteralArray(&args);

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
					Toy_freeLiteral(keycodeLiteral);
					break;
				}

				//get the event name
				Toy_Literal eventLiteral = Toy_getLiteralDictionary(&engine.symKeyDownEvents, keycodeLiteral);

				//call the function
				Toy_pushLiteralArray(&args, eventLiteral);
				Box_callRecursiveEngineNode(engine.rootNode, &engine.interpreter, "onKeyDown", &args);
				Toy_popLiteralArray(&args);

				//push to the event list
				Toy_freeLiteral(eventLiteral);
				Toy_freeLiteral(keycodeLiteral);
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
					Toy_freeLiteral(keycodeLiteral);
					break;
				}

				//get the event name
				Toy_Literal eventLiteral = Toy_getLiteralDictionary(&engine.symKeyUpEvents, keycodeLiteral);

				//call the function
				Toy_pushLiteralArray(&args, eventLiteral);
				Box_callRecursiveEngineNode(engine.rootNode, &engine.interpreter, "onKeyUp", &args);
				Toy_popLiteralArray(&args);

				//push to the event list
				Toy_freeLiteral(eventLiteral);
				Toy_freeLiteral(keycodeLiteral);
			}
			break;

			//mouse motion
			case SDL_MOUSEMOTION: {
				Toy_Literal mouseX = TOY_TO_INTEGER_LITERAL( (int)(event.motion.x) );
				Toy_Literal mouseY = TOY_TO_INTEGER_LITERAL( (int)(event.motion.y) );
				Toy_Literal mouseXRel = TOY_TO_INTEGER_LITERAL( (int)(event.motion.xrel) );
				Toy_Literal mouseYRel = TOY_TO_INTEGER_LITERAL( (int)(event.motion.yrel) );

				Toy_pushLiteralArray(&args, mouseX);
				Toy_pushLiteralArray(&args, mouseY);
				Toy_pushLiteralArray(&args, mouseXRel);
				Toy_pushLiteralArray(&args, mouseYRel);

				Box_callRecursiveEngineNode(engine.rootNode, &engine.interpreter, "onMouseMotion", &args);

				Toy_freeLiteral(mouseX);
				Toy_freeLiteral(mouseY);
				Toy_freeLiteral(mouseXRel);
				Toy_freeLiteral(mouseYRel);

				//hack: manual free
				for(int i = 0; i < args.count; i++) {
					Toy_freeLiteral(args.literals[i]);
				}
				args.count = 0;
			}
			break;

			//mouse button down
			case SDL_MOUSEBUTTONDOWN: {
				Toy_Literal mouseX = TOY_TO_INTEGER_LITERAL( (int)(event.button.x) );
				Toy_Literal mouseY = TOY_TO_INTEGER_LITERAL( (int)(event.button.y) );
				Toy_Literal mouseButton;

				switch (event.button.button) {
					case SDL_BUTTON_LEFT:
						mouseButton = TOY_TO_STRING_LITERAL(Toy_createRefString("left"));
						break;

					case SDL_BUTTON_MIDDLE:
						mouseButton = TOY_TO_STRING_LITERAL(Toy_createRefString("middle"));
						break;

					case SDL_BUTTON_RIGHT:
						mouseButton = TOY_TO_STRING_LITERAL(Toy_createRefString("right"));
						break;

					case SDL_BUTTON_X1:
						mouseButton = TOY_TO_STRING_LITERAL(Toy_createRefString("x1"));
						break;

					case SDL_BUTTON_X2:
						mouseButton = TOY_TO_STRING_LITERAL(Toy_createRefString("x2"));
						break;

					default:
						mouseButton = TOY_TO_STRING_LITERAL(Toy_createRefString("unknown"));
						break;
				}

				Toy_pushLiteralArray(&args, mouseX);
				Toy_pushLiteralArray(&args, mouseY);
				Toy_pushLiteralArray(&args, mouseButton);

				Box_callRecursiveEngineNode(engine.rootNode, &engine.interpreter, "onMouseButtonDown", &args);

				Toy_freeLiteral(mouseX);
				Toy_freeLiteral(mouseY);
				Toy_freeLiteral(mouseButton);

				//hack: manual free
				for(int i = 0; i < args.count; i++) {
					Toy_freeLiteral(args.literals[i]);
				}
				args.count = 0;
			}
			break;

			//mouse button up
			case SDL_MOUSEBUTTONUP: {
				Toy_Literal mouseX = TOY_TO_INTEGER_LITERAL( (int)(event.button.x) );
				Toy_Literal mouseY = TOY_TO_INTEGER_LITERAL( (int)(event.button.y) );
				Toy_Literal mouseButton;

				switch (event.button.button) {
					case SDL_BUTTON_LEFT:
						mouseButton = TOY_TO_STRING_LITERAL(Toy_createRefString("left"));
						break;

					case SDL_BUTTON_MIDDLE:
						mouseButton = TOY_TO_STRING_LITERAL(Toy_createRefString("middle"));
						break;

					case SDL_BUTTON_RIGHT:
						mouseButton = TOY_TO_STRING_LITERAL(Toy_createRefString("right"));
						break;

					case SDL_BUTTON_X1:
						mouseButton = TOY_TO_STRING_LITERAL(Toy_createRefString("x1"));
						break;

					case SDL_BUTTON_X2:
						mouseButton = TOY_TO_STRING_LITERAL(Toy_createRefString("x2"));
						break;

					default:
						mouseButton = TOY_TO_STRING_LITERAL(Toy_createRefString("unknown"));
						break;
				}

				Toy_pushLiteralArray(&args, mouseX);
				Toy_pushLiteralArray(&args, mouseY);
				Toy_pushLiteralArray(&args, mouseButton);

				Box_callRecursiveEngineNode(engine.rootNode, &engine.interpreter, "onMouseButtonUp", &args);

				Toy_freeLiteral(mouseX);
				Toy_freeLiteral(mouseY);
				Toy_freeLiteral(mouseButton);

				//hack: manual free
				for(int i = 0; i < args.count; i++) {
					Toy_freeLiteral(args.literals[i]);
				}
				args.count = 0;
			}
			break; //TODO: remove copied code

			//mouse wheel
			case SDL_MOUSEWHEEL: {
				Toy_Literal mouseX = TOY_TO_INTEGER_LITERAL( (int)(event.wheel.x) );
				Toy_Literal mouseY = TOY_TO_INTEGER_LITERAL( (int)(event.wheel.y) );
				Toy_pushLiteralArray(&args, mouseX);
				Toy_pushLiteralArray(&args, mouseY);

				Box_callRecursiveEngineNode(engine.rootNode, &engine.interpreter, "onMouseWheel", &args);

				Toy_freeLiteral(mouseX);
				Toy_freeLiteral(mouseY);

				//hack: manual free
				for(int i = 0; i < args.count; i++) {
					Toy_freeLiteral(args.literals[i]);
				}
				args.count = 0;
			}
			break;
		}
	}

	Toy_freeLiteralArray(&args);
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
