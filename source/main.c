//moved here for android shenanigans
#define SDL_MAIN_HANDLED

#include "box_engine.h"

//the runner library needs a little more setup since it accesses the disk
#include "lib_runner.h"

int main(int argc, char* argv[]) {
	//the drive system uses a LiteralDictionary, which must be initialized with this
	Toy_initDriveDictionary();

	{
		//create a pair of literals, the first for the drive name, the second for the path
		Toy_Literal driveLiteral = TOY_TO_STRING_LITERAL(Toy_createRefString("scripts"));
		Toy_Literal pathLiteral = TOY_TO_STRING_LITERAL(Toy_createRefString("assets/scripts"));

		//set these within the drive dictionary
		Toy_setLiteralDictionary(Toy_getDriveDictionary(), driveLiteral, pathLiteral);

		//these literals are no longer needed
		Toy_freeLiteral(driveLiteral);
		Toy_freeLiteral(pathLiteral);
	}

	{
		//create a pair of literals, the first for the drive name, the second for the path
		Toy_Literal driveLiteral = TOY_TO_STRING_LITERAL(Toy_createRefString("sprites"));
		Toy_Literal pathLiteral = TOY_TO_STRING_LITERAL(Toy_createRefString("assets/sprites"));

		//set these within the drive dictionary
		Toy_setLiteralDictionary(Toy_getDriveDictionary(), driveLiteral, pathLiteral);

		//these literals are no longer needed
		Toy_freeLiteral(driveLiteral);
		Toy_freeLiteral(pathLiteral);
	}

	//run the rest of your program
	Box_initEngine();
	Box_execEngine();
	Box_freeEngine();

	//clean up the drive dictionary when you're done
	Toy_freeDriveDictionary();

	return 0;
}
