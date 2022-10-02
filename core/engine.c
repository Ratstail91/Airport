#include "engine.h"

#include "lib_engine.h"
#include "lib_standard.h"

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

//compilation functions
//TODO: move these to their own file
static char* readFile(char* path, size_t* fileSize) {
	FILE* file = fopen(path, "rb");

	if (file == NULL) {
		fprintf(stderr, ERROR "Could not open file \"%s\"\n" RESET, path);
		exit(-1);
	}

	fseek(file, 0L, SEEK_END);
	*fileSize = ftell(file);
	rewind(file);

	char* buffer = (char*)malloc(*fileSize + 1);

	if (buffer == NULL) {
		fprintf(stderr, ERROR "Not enough memory to read \"%s\"\n" RESET, path);
		exit(-1);
	}

	size_t bytesRead = fread(buffer, sizeof(char), *fileSize, file);

	buffer[*fileSize] = '\0'; //NOTE: fread doesn't append this

	if (bytesRead < *fileSize) {
		fprintf(stderr, ERROR "Could not read file \"%s\"\n" RESET, path);
		exit(-1);
	}

	fclose(file);

	return buffer;
}

static unsigned char* compileString(char* source, size_t* size) {
	Lexer lexer;
	Parser parser;
	Compiler compiler;

	initLexer(&lexer, source);
	initParser(&parser, &lexer);
	initCompiler(&compiler);

	//run the parser until the end of the source
	ASTNode* node = scanParser(&parser);
	while(node != NULL) {
		//pack up and leave
		if (node->type == AST_NODEERROR) {
			printf(ERROR "error node detected\n" RESET);
			freeNode(node);
			freeCompiler(&compiler);
			freeParser(&parser);
			return NULL;
		}

		writeCompiler(&compiler, node);
		freeNode(node);
		node = scanParser(&parser);
	}

	//get the bytecode dump
	unsigned char* tb = collateCompiler(&compiler, (int*)(size));

	//cleanup
	freeCompiler(&compiler);
	freeParser(&parser);
	//no lexer to clean up

	//finally
	return tb;
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
		FREE(EngineNode, engine.rootNode);

		engine.rootNode = NULL;
	}

	freeInterpreter(&engine.interpreter);

	engine.renderer = NULL;
	engine.window = NULL;
}

static void execStep() {
	//call onStep
	callEngineNode(engine.rootNode, &engine.interpreter, "onStep");

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

		//if not enough time has passed
		if (engine.simTime.tv_sec < engine.realTime.tv_sec && engine.simTime.tv_usec < engine.realTime.tv_usec) {
			//while not enough time has passed
			while(engine.simTime.tv_sec < engine.realTime.tv_sec && engine.simTime.tv_usec < engine.realTime.tv_usec) {
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
		SDL_RenderPresent(engine.renderer);
	}
}
