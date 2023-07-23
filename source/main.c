#include "box_engine.h"

int main(int argc, char* argv[]) {
	//debugging tools
#ifdef _DEBUG
	// Memory Leak Detection during Debug Builds (MSVC only)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif//win32 && debug

	//the drive system maps filepaths to "drives", which specifies which folders can be accessed by the scripts
	Toy_initDriveSystem();

	// Toy_setDrivePath("airport", "assets/airport-demo"); //DEBUG
	Toy_setDrivePath("scripts", "assets/scripts");
	Toy_setDrivePath("sprites", "assets/sprites");
	Toy_setDrivePath("audio", "assets/audio");
	Toy_setDrivePath("music", "assets/audio/music");
	Toy_setDrivePath("sound", "assets/audio/sound");
	Toy_setDrivePath("fonts", "assets/fonts");

	Box_initEngine("scripts:/init.toy");
	Box_execEngine();
	Box_freeEngine();

	//clean up the drive dictionary when you're done
	Toy_freeDriveSystem();

	return 0;
}
