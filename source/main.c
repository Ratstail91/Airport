//moved here for android shenanigans
#define SDL_MAIN_HANDLED

#include "engine.h"

int main(int argc, char* argv[]) {
	initEngine();
	execEngine();
	freeEngine();
	return 0;
}
