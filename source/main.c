#include "engine.h"

int main(int argc, char* argv[]) {
	Engine engine = { .screenWidth = 800, .screenHeight = 600 };

	initEngine(&engine);
	execEngine(&engine);
	freeEngine(&engine);

	return 0;
}
