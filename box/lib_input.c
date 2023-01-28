#include "lib_input.h"

#include "box_common.h"
#include "box_engine.h"

#include "toy_memory.h"

static int nativeMapInputEventToKey(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments, Toy_LiteralDictionary* symKeyEventsPtr, char* fnName) {
	//checks
	if (arguments->count != 2) {
		interpreter->errorOutput("Incorrect number of arguments passed to ");
		interpreter->errorOutput(fnName);
		interpreter->errorOutput("\n");
		return -1;
	}

	Toy_Literal symLiteral = Toy_popLiteralArray(arguments);
	Toy_Literal evtLiteral = Toy_popLiteralArray(arguments);

	Toy_Literal evtLiteralIdn = evtLiteral;
	if (TOY_IS_IDENTIFIER(evtLiteral) && Toy_parseIdentifierToValue(interpreter, &evtLiteral)) {
		Toy_freeLiteral(evtLiteralIdn);
	}

	Toy_Literal symLiteralIdn = symLiteral;
	if (TOY_IS_IDENTIFIER(symLiteral) && Toy_parseIdentifierToValue(interpreter, &symLiteral)) {
		Toy_freeLiteral(symLiteralIdn);
	}

	if (!TOY_IS_STRING(symLiteral) || !TOY_IS_STRING(evtLiteral)) {
		interpreter->errorOutput("Incorrect type of arguments passed to mapInputEventToKey\n");
		return -1;
	}

	//use the keycode for faster lookups
	SDL_Keycode keycode = SDL_GetKeyFromName( Toy_toCString(TOY_AS_STRING(symLiteral)) );

	if (keycode == SDLK_UNKNOWN) {
		interpreter->errorOutput("Unknown key found: ");
		interpreter->errorOutput(SDL_GetError());
		interpreter->errorOutput("\n");
		return -1;
	}

	Toy_Literal keycodeLiteral = TOY_TO_INTEGER_LITERAL( (int)keycode );

	//save the sym-event pair
	Toy_setLiteralDictionary(symKeyEventsPtr, keycodeLiteral, evtLiteral); //I could possibly map multiple events to one sym

	//cleanup
	Toy_freeLiteral(symLiteral);
	Toy_freeLiteral(evtLiteral);
	Toy_freeLiteral(keycodeLiteral);

	return 0;
}

//dry wrappers
static int nativeMapInputEventToKeyDown(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	return nativeMapInputEventToKey(interpreter, arguments, &engine.symKeyDownEvents, "mapInputEventToKeyDown");
}

static int nativeMapInputEventToKeyUp(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	return nativeMapInputEventToKey(interpreter, arguments, &engine.symKeyUpEvents, "mapInputEventToKeyUp");
}

//call the hook
typedef struct Natives {
	char* name;
	Toy_NativeFn fn;
} Natives;

int Box_hookInput(Toy_Interpreter* interpreter, Toy_Literal identifier, Toy_Literal alias) {
	//build the natives list
	Natives natives[] = {
		{"mapInputEventToKeyDown", nativeMapInputEventToKeyDown},
		{"mapInputEventToKeyUp", nativeMapInputEventToKeyUp},
		// {"mapInputEventToMouse", nativeMapInputEventToMouse},
		{NULL, NULL}
	};

	//store the library in an aliased dictionary
	if (!TOY_IS_NULL(alias)) {
		//make sure the name isn't taken
		if (Toy_isDelcaredScopeVariable(interpreter->scope, alias)) {
			interpreter->errorOutput("Can't override an existing variable\n");
			Toy_freeLiteral(alias);
			return false;
		}

		//create the dictionary to load up with functions
		Toy_LiteralDictionary* dictionary = TOY_ALLOCATE(Toy_LiteralDictionary, 1);
		Toy_initLiteralDictionary(dictionary);

		//load the dict with functions
		for (int i = 0; natives[i].name; i++) {
			Toy_Literal name = TOY_TO_STRING_LITERAL(Toy_createRefString(natives[i].name));
			Toy_Literal func = TOY_TO_FUNCTION_NATIVE_LITERAL(natives[i].fn);

			Toy_setLiteralDictionary(dictionary, name, func);

			Toy_freeLiteral(name);
			Toy_freeLiteral(func);
		}

		//build the type
		Toy_Literal type = TOY_TO_TYPE_LITERAL(TOY_LITERAL_DICTIONARY, true);
		Toy_Literal strType = TOY_TO_TYPE_LITERAL(TOY_LITERAL_STRING, true);
		Toy_Literal fnType = TOY_TO_TYPE_LITERAL(TOY_LITERAL_FUNCTION_NATIVE, true);
		TOY_TYPE_PUSH_SUBTYPE(&type, strType);
		TOY_TYPE_PUSH_SUBTYPE(&type, fnType);

		//set scope
		Toy_Literal dict = TOY_TO_DICTIONARY_LITERAL(dictionary);
		Toy_declareScopeVariable(interpreter->scope, alias, type);
		Toy_setScopeVariable(interpreter->scope, alias, dict, false);

		//cleanup
		Toy_freeLiteral(dict);
		Toy_freeLiteral(type);
		return 0;
	}

	//default
	for (int i = 0; natives[i].name; i++) {
		Toy_injectNativeFn(interpreter, natives[i].name, natives[i].fn);
	}

	return 0;
}
