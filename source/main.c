//moved here for android shenanigans
#define SDL_MAIN_HANDLED

#include "engine.h"

//the runner library needs a little more setup since it accesses the disk
#include "lib_runner.h"

int main(int argc, char* argv[]) {
	//the drive system uses a LiteralDictionary, which must be initialized with this
    initDriveDictionary();

    //create a pair of literals, the first for the drive name, the second for the path
    Literal driveLiteral = TO_STRING_LITERAL(createRefString("scripts"));
    Literal pathLiteral = TO_STRING_LITERAL(createRefString("assets/scripts"));

    //set these within the drive dictionary
    setLiteralDictionary(getDriveDictionary(), driveLiteral, pathLiteral);

    //these literals are no longer needed
    freeLiteral(driveLiteral);
    freeLiteral(pathLiteral);

    //run the rest of your program
	initEngine();
	execEngine();
	freeEngine();

    //clean up the drive dictionary when you're done
    freeDriveDictionary();

	return 0;
}
