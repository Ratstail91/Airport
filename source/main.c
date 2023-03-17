//moved here for android shenanigans
#define SDL_MAIN_HANDLED

#include "box_engine.h"
#include "toy_drive_system.h"

int main(int argc, char* argv[]) {
	//debugging tools
#ifdef _DEBUG
	// Memory Leak Detection during Debug Builds (MSVC only)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif//win32 && debug

	//the drive system uses a LiteralDictionary, which must be initialized with this
	Toy_initDriveSystem();

	Toy_setDrivePath("scripts", "assets/scripts");
	Toy_setDrivePath("sprites", "assets/sprites");
	Toy_setDrivePath("fonts", "assets/fonts");

	Box_initEngine();
	Box_execEngine();
	Box_freeEngine();

	//clean up the drive dictionary when you're done
	Toy_freeDriveSystem();

	return 0;
}
