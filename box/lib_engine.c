#include "lib_engine.h"

#include "box_engine.h"

#include "repl_tools.h"
#include "toy_memory.h"
#include "toy_literal_array.h"

#include "toy_console_colors.h"

#include "lib_runner.h"

#include <stdio.h>

//errors here should be fatal
static void fatalError(char* message) {
	fprintf(stderr, TOY_CC_ERROR "%s" TOY_CC_RESET, message);
	exit(-1);
}

//native functions to be called
static int nativeInitWindow(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	if (engine.window != NULL) {
		fatalError("Can't re-initialize the window\n");
	}

	if (arguments->count != 4) {
		fatalError("Incorrect number of arguments passed to initEngine\n");
	}

	//extract the arguments
	Toy_Literal fscreen = Toy_popLiteralArray(arguments);
	Toy_Literal screenHeight = Toy_popLiteralArray(arguments);
	Toy_Literal screenWidth = Toy_popLiteralArray(arguments);
	Toy_Literal caption = Toy_popLiteralArray(arguments);

	//check argument types
	if (!TOY_IS_STRING(caption) || !TOY_IS_INTEGER(screenWidth) || !TOY_IS_INTEGER(screenHeight) || !TOY_IS_BOOLEAN(fscreen)) {
		fatalError("Incorrect argument type passed to initEngine\n");
	}

	//init the window
	engine.window = SDL_CreateWindow(
		Toy_toCString(TOY_AS_STRING(caption)),
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		engine.screenWidth = TOY_AS_INTEGER(screenWidth),
		engine.screenHeight = TOY_AS_INTEGER(screenHeight),
		TOY_IS_TRUTHY(fscreen) ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE
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

	Toy_freeLiteral(caption);
	Toy_freeLiteral(screenWidth);
	Toy_freeLiteral(screenHeight);
	Toy_freeLiteral(fscreen);

	return 0;
}

//TODO: perhaps a returns argument would be better?
static int nativeLoadRootNode(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to loadRootNode\n");
		return -1;
	}

	//extract the arguments
	Toy_Literal drivePathLiteral = Toy_popLiteralArray(arguments);

	Toy_Literal drivePathLiteralIdn = drivePathLiteral;
	if (TOY_IS_IDENTIFIER(drivePathLiteral) && Toy_parseIdentifierToValue(interpreter, &drivePathLiteral)) {
		Toy_freeLiteral(drivePathLiteralIdn);
	}

	//check argument types
	if (!TOY_IS_STRING(drivePathLiteral)) {
		interpreter->errorOutput("Incorrect argument type passed to loadRootNode\n");
		Toy_freeLiteral(drivePathLiteral);
		return -1;
	}

	Toy_Literal filePathLiteral = Toy_getFilePathLiteral(interpreter, &drivePathLiteral);

	Toy_freeLiteral(drivePathLiteral); //not needed anymore

	if (!TOY_IS_STRING(filePathLiteral)) {
		Toy_freeLiteral(filePathLiteral);
		return -1;
	}

	//clear existing root node
	if (engine.rootNode != NULL) {
		Box_callRecursiveEngineNode(engine.rootNode, &engine.interpreter, "onFree", NULL);

		Box_freeEngineNode(engine.rootNode);
		TOY_FREE(Box_EngineNode, engine.rootNode);

		engine.rootNode = NULL;
	}

	//load the new root node
	size_t size = 0;
	const unsigned char* source = Toy_readFile(Toy_toCString(TOY_AS_STRING(filePathLiteral)), &size);
	const unsigned char* tb = Toy_compileString((const char*)source, &size);
	free((void*)source);

	engine.rootNode = TOY_ALLOCATE(Box_EngineNode, 1);

	//BUGFIX: make an inner-interpreter
	Toy_Interpreter inner;

	//init the inner interpreter manually
	Toy_initLiteralArray(&inner.literalCache);
	inner.scope = Toy_pushScope(NULL);
	inner.bytecode = tb;
	inner.length = size;
	inner.count = 0;
	inner.codeStart = -1;
	inner.depth = interpreter->depth + 1;
	inner.panic = false;
	Toy_initLiteralArray(&inner.stack);
	inner.hooks = interpreter->hooks;
	Toy_setInterpreterPrint(&inner, interpreter->printOutput);
	Toy_setInterpreterAssert(&inner, interpreter->assertOutput);
	Toy_setInterpreterError(&inner, interpreter->errorOutput);

	Box_initEngineNode(engine.rootNode, &inner, tb, size);

	//init the new node (and ONLY this node)
	Box_callEngineNode(engine.rootNode, &engine.interpreter, "onInit", NULL);

	//cleanup
	while(inner.scope) {
		inner.scope = Toy_popScope(inner.scope);
	}

	Toy_freeLiteralArray(&inner.stack);
	Toy_freeLiteralArray(&inner.literalCache);
	Toy_freeLiteral(filePathLiteral);

	return 0;
}

//call the hook
typedef struct Natives {
	char* name;
	Toy_NativeFn fn;
} Natives;

int Box_hookEngine(Toy_Interpreter* interpreter, Toy_Literal identifier, Toy_Literal alias) {
	//build the natives list
	Natives natives[] = {
		{"initWindow", nativeInitWindow},
		{"loadRootNode", nativeLoadRootNode},
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
