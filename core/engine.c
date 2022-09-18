#include "engine.h"

#include <stdio.h>

//errors here should be fatal
static void error(Engine* engine, char* message) {
	fprintf(stderr, message);
	exit(-1);
}

//exposed functions
void initEngine(Engine* engine) {
	//clear
	engine->root = NULL;
	engine->running = true;

	//init SDL
	if (SDL_Init(0) != 0) {
		error(engine, "Failed to initialize SDL2");
	}

	//init the window
	engine->window = SDL_CreateWindow(
		"Caption",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		engine->screenWidth,
		engine->screenHeight,
		SDL_WINDOW_RESIZABLE
	);

	if (engine->window == NULL) {
		error(engine, "Failed to initialize the window");
	}

	//init the renderer
	engine->renderer = SDL_CreateRenderer(engine->window, -1, 0);

	if (engine->renderer == NULL) {
		error(engine, "Failed to initialize the renderer");
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
	SDL_RenderSetLogicalSize(engine->renderer, engine->screenWidth, engine->screenHeight);
}

void freeEngine(Engine* engine) {
	SDL_DestroyRenderer(engine->renderer);
	SDL_DestroyWindow(engine->window);
	SDL_Quit();

	engine->renderer = NULL;
	engine->window = NULL;
}

static void execStep(Engine* engine) {
	//DEBUG: for now, just poll events
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch(event.type) {
			//quit
			case SDL_QUIT: {
				engine->running = false;
			}
			break;

			//window events are handled internally
			case SDL_WINDOWEVENT: {
				switch(event.window.event) {
					case SDL_WINDOWEVENT_RESIZED:
						SDL_RenderSetLogicalSize(engine->renderer, event.window.data1, event.window.data2);
					break;
				}
			}
			break;

			//TODO: input
		}
	}
}

//the heart of the engine
void execEngine(Engine* engine) {
	//set up time
	gettimeofday(&engine->realTime, NULL);
	engine->simTime = engine->realTime;
	struct timeval delta = { .tv_sec = 0, .tv_usec = 1000 * 1000 / 60 }; //60 frames per second

	while (engine->running) {
		//calc the time passed
		gettimeofday(&engine->realTime, NULL);

		//if not enough time has passed
		if (engine->simTime.tv_sec < engine->realTime.tv_sec && engine->simTime.tv_usec < engine->realTime.tv_usec) {
			//while not enough time has passed
			while(engine->simTime.tv_sec < engine->realTime.tv_sec && engine->simTime.tv_usec < engine->realTime.tv_usec) {
				//simulate the world
				execStep(engine);

				//calc the time simulation
				timeradd(&delta, &engine->simTime, &engine->simTime);
			}
		}
		else {
			SDL_Delay(10); //let the machine sleep, 10ms
		}

		//render the world
		SDL_SetRenderDrawColor(engine->renderer, 0, 0, 0, 255); //NOTE: This line can be disabled later
		SDL_RenderClear(engine->renderer); //NOTE: This line can be disabled later
		SDL_RenderPresent(engine->renderer);
	}
}
