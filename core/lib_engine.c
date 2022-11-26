#include "lib_engine.h"

#include "engine.h"
#include "repl_tools.h"

#include "memory.h"
#include "literal_array.h"

//errors here should be fatal
static void fatalError(char* message) {
	fprintf(stderr, "%s", message);
	exit(-1);
}

//native functions to be called
static int nativeInitWindow(Interpreter* interpreter, LiteralArray* arguments) {
	if (engine.window != NULL) {
		fatalError("Can't re-initialize the window\n");
	}

	if (arguments->count != 4) {
		fatalError("Incorrect number of arguments passed to initEngine\n");
	}

	//extract the arguments
	Literal fscreen = popLiteralArray(arguments);
	Literal screenHeight = popLiteralArray(arguments);
	Literal screenWidth = popLiteralArray(arguments);
	Literal caption = popLiteralArray(arguments);

	//check argument types
	if (!IS_STRING(caption) || !IS_INTEGER(screenWidth) || !IS_INTEGER(screenHeight) || !IS_BOOLEAN(fscreen)) {
		fatalError("Incorrect argument type passed to initEngine\n");
	}

	//init the window
	engine.window = SDL_CreateWindow(
		toCString(AS_STRING(caption)),
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		engine.screenWidth = AS_INTEGER(screenWidth),
		engine.screenHeight = AS_INTEGER(screenHeight),
		IS_TRUTHY(fscreen) ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE
	);

	if (engine.window == NULL) {
		fatalError("Failed to initialize the window\n");
	}

	//init the renderer
	// SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
	engine.renderer = SDL_CreateRenderer(engine.window, -1, 0);

	if (engine.renderer == NULL) {
		fatalError("Failed to initialize the renderer\n");
	}

	SDL_RendererInfo rendererInfo;
	SDL_GetRendererInfo(engine.renderer, &rendererInfo);

	printf("Renderer: %s\n", rendererInfo.name);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
	SDL_RenderSetLogicalSize(engine.renderer, engine.screenWidth, engine.screenHeight);

	//only run with a window
	engine.running = true;

	freeLiteral(caption);
	freeLiteral(screenWidth);
	freeLiteral(screenHeight);
	freeLiteral(fscreen);

	return 0;
}

//TODO: perhaps a returns argument would be better?
static int nativeLoadRootNode(Interpreter* interpreter, LiteralArray* arguments) {
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to loadRootNode\n");
		return -1;
	}

	//extract the arguments
	Literal fname = popLiteralArray(arguments);

	Literal fnameIdn = fname;
	if (IS_IDENTIFIER(fname) && parseIdentifierToValue(interpreter, &fname)) {
		freeLiteral(fnameIdn);
	}

	//check argument types
	if (!IS_STRING(fname)) {
		interpreter->errorOutput("Incorrect argument type passed to loadRootNode\n");
		freeLiteral(fname);
		return -1;
	}

	//clear existing root node
	if (engine.rootNode != NULL) {
		callRecursiveEngineNode(engine.rootNode, &engine.interpreter, "onFree", NULL);

		freeEngineNode(engine.rootNode);
		FREE(EngineNode, engine.rootNode);

		engine.rootNode = NULL;
	}

	//load the new root node
	size_t size = 0;
	char* source = readFile(toCString(AS_STRING(fname)), &size);
	unsigned char* tb = compileString(source, &size);
	free((void*)source);

	engine.rootNode = ALLOCATE(EngineNode, 1);

	//BUGFIX: make an inner-interpreter
	Interpreter inner;

	//init the inner interpreter manually
	initLiteralArray(&inner.literalCache);
	inner.scope = pushScope(NULL);
	inner.bytecode = tb;
	inner.length = size;
	inner.count = 0;
	inner.codeStart = -1;
	inner.depth = interpreter->depth + 1;
	inner.panic = false;
	initLiteralArray(&inner.stack);
	inner.exports = interpreter->exports;
	inner.exportTypes = interpreter->exportTypes;
	inner.hooks = interpreter->hooks;
	setInterpreterPrint(&inner, interpreter->printOutput);
	setInterpreterAssert(&inner, interpreter->assertOutput);
	setInterpreterError(&inner, interpreter->errorOutput);

	initEngineNode(engine.rootNode, &inner, tb, size);

	//init the new node (and ONLY this node)
	callEngineNode(engine.rootNode, &engine.interpreter, "onInit", NULL);

	//cleanup
	freeLiteralArray(&inner.stack);
	freeLiteralArray(&inner.literalCache);
	freeLiteral(fname);

	return 0;
}

//call the hook
typedef struct Natives {
	char* name;
	NativeFn fn;
} Natives;

int hookEngine(Interpreter* interpreter, Literal identifier, Literal alias) {
	//build the natives list
	Natives natives[] = {
		{"initWindow", nativeInitWindow},
		{"loadRootNode", nativeLoadRootNode},
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
			Literal name = TO_STRING_LITERAL(createRefString(natives[i].name));
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
