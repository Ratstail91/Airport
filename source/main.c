#include "engine.h"

int main(int argc, char* argv[]) {
	Engine engine = { .screenWidth = 640, .screenHeight = 480 };

	initEngine(&engine);
	execEngine(&engine);
	freeEngine(&engine);

	return 0;
}