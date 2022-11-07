#include "lib_input.h"

#include "memory.h"

#include "engine.h"
#include "core_common.h"

static int nativeMapInputEventToKey(Interpreter* interpreter, LiteralArray* arguments, LiteralDictionary* symKeyEventsPtr, char* fnName) {
	//checks
	if (arguments->count != 2) {
		interpreter->errorOutput("Incorrect number of arguments passed to ");
		interpreter->errorOutput(fnName);
		interpreter->errorOutput("\n");
		return -1;
	}

	Literal symLiteral = popLiteralArray(arguments);
	Literal evtLiteral = popLiteralArray(arguments);

	Literal evtLiteralIdn = evtLiteral;
	if (IS_IDENTIFIER(evtLiteral) && parseIdentifierToValue(interpreter, &evtLiteral)) {
		freeLiteral(evtLiteralIdn);
	}

	Literal symLiteralIdn = symLiteral;
	if (IS_IDENTIFIER(symLiteral) && parseIdentifierToValue(interpreter, &symLiteral)) {
		freeLiteral(symLiteralIdn);
	}

	if (!IS_STRING(symLiteral) || !IS_STRING(evtLiteral)) {
		interpreter->errorOutput("Incorrect type of arguments passed to mapInputEventToKey\n");
		return -1;
	}

	//use the keycode for faster lookups
	SDL_Keycode keycode = SDL_GetKeyFromName( AS_STRING(symLiteral) );

	if (keycode == SDLK_UNKNOWN) {
		interpreter->errorOutput("Unknown key found: ");
		interpreter->errorOutput(SDL_GetError());
		interpreter->errorOutput("\n");
		return -1;
	}

	Literal keycodeLiteral = TO_INTEGER_LITERAL( (int)keycode );

	//save the sym-event pair
	setLiteralDictionary(symKeyEventsPtr, keycodeLiteral, evtLiteral); //I could possibly map multiple events to one sym

	//cleanup
	freeLiteral(symLiteral);
	freeLiteral(evtLiteral);
	freeLiteral(keycodeLiteral);

	return 0;
}

//dry wrappers
static int nativeMapInputEventToKeyDown(Interpreter* interpreter, LiteralArray* arguments) {
	return nativeMapInputEventToKey(interpreter, arguments, &engine.symKeyDownEvents, "mapInputEventToKeyDown");
}

static int nativeMapInputEventToKeyUp(Interpreter* interpreter, LiteralArray* arguments) {
	return nativeMapInputEventToKey(interpreter, arguments, &engine.symKeyUpEvents, "mapInputEventToKeyUp");
}

//call the hook
typedef struct Natives {
	char* name;
	NativeFn fn;
} Natives;

int hookInput(Interpreter* interpreter, Literal identifier, Literal alias) {
	//build the natives list
	Natives natives[] = {
		{"mapInputEventToKeyDown", nativeMapInputEventToKeyDown},
		{"mapInputEventToKeyUp", nativeMapInputEventToKeyUp},
		// {"mapInputEventToMouse", nativeMapInputEventToMouse},
		{NULL, NULL}
	};

	//store the library in an aliased dictionary
	if (!IS_NULL(alias)) {
		//make sure the name isn't taken
		if (isDelcaredScopeVariable(interpreter->scope, alias)) {
			interpreter->errorOutput("Can't override an existing variable\n");
			freeLiteral(alias);
			return false;
		}

		//create the dictionary to load up with functions
		LiteralDictionary* dictionary = ALLOCATE(LiteralDictionary, 1);
		initLiteralDictionary(dictionary);

		//load the dict with functions
		for (int i = 0; natives[i].name; i++) {
			Literal name = TO_STRING_LITERAL(copyString(natives[i].name, strlen(natives[i].name)), strlen(natives[i].name));
			Literal func = TO_FUNCTION_LITERAL((void*)natives[i].fn, 0);
			func.type = LITERAL_FUNCTION_NATIVE;

			setLiteralDictionary(dictionary, name, func);

			freeLiteral(name);
			freeLiteral(func);
		}

		//build the type
		Literal type = TO_TYPE_LITERAL(LITERAL_DICTIONARY, true);
		Literal strType = TO_TYPE_LITERAL(LITERAL_STRING, true);
		Literal fnType = TO_TYPE_LITERAL(LITERAL_FUNCTION_NATIVE, true);
		TYPE_PUSH_SUBTYPE(&type, strType);
		TYPE_PUSH_SUBTYPE(&type, fnType);

		//set scope
		Literal dict = TO_DICTIONARY_LITERAL(dictionary);
		declareScopeVariable(interpreter->scope, alias, type);
		setScopeVariable(interpreter->scope, alias, dict, false);

		//cleanup
		freeLiteral(dict);
		freeLiteral(type);
		return 0;
	}

	//default
	for (int i = 0; natives[i].name; i++) {
		injectNativeFn(interpreter, natives[i].name, natives[i].fn);
	}

	return 0;
}
