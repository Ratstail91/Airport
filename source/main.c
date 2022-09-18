//This "hello world" was borrowed from the net
//https://gist.github.com/fschr/92958222e35a823e738bb181fe045274

// SDL2 Hello, World!
// This should display a white screen for 2 seconds
// compile with: clang++ main.cpp -o hello_sdl2 -lSDL2
// run with: ./hello_sdl2
#include <SDL2/SDL.h>
#include <stdio.h>

#include "lexer.h"
#include "parser.h"
#include "compiler.h"
#include "interpreter.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

int main(int argc, char* args[]) {
  SDL_Window* window = NULL;
  SDL_Surface* screenSurface = NULL;
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "could not initialize sdl2: %s\n", SDL_GetError());
    return 1;
  }
  window = SDL_CreateWindow(
			    "hello_sdl2",
			    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			    SCREEN_WIDTH, SCREEN_HEIGHT,
			    SDL_WINDOW_SHOWN
			    );
  if (window == NULL) {
    fprintf(stderr, "could not create window: %s\n", SDL_GetError());
    return 1;
  }
  screenSurface = SDL_GetWindowSurface(window);
  SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF));
  SDL_UpdateWindowSurface(window);

	//hacked in to test linking
	{
		//source
		char* source = "print \"Hello world!\";";

		//test basic compilation & collation
		Lexer lexer;
		Parser parser;
		Compiler compiler;
		Interpreter interpreter;

		initLexer(&lexer, source);
		initParser(&parser, &lexer);
		initCompiler(&compiler);
		initInterpreter(&interpreter);

		Node* node = scanParser(&parser);

		//write
		writeCompiler(&compiler, node);

		//collate
		int size = 0;
		unsigned char* bytecode = collateCompiler(&compiler, &size);

		//run
		runInterpreter(&interpreter, bytecode, size);

		//cleanup
		freeNode(node);
		freeParser(&parser);
		freeCompiler(&compiler);
		freeInterpreter(&interpreter);
	}

	SDL_Delay(2000);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}