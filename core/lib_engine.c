#include "lib_engine.h"

#include "engine.h"
#include "repl_tools.h"

#include "memory.h"
#include "literal_array.h"

//errors here should be fatal
static void fatalError(char* message) {
	fprintf(stderr, message);
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
		AS_STRING(caption),
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
		callEngineNode(engine.rootNode, &engine.interpreter, "onFree");

		freeEngineNode(engine.rootNode);
		FREE(EngineNode, engine.rootNode);

		engine.rootNode = NULL;
	}

	//load the new root node
	size_t size = 0;
	char* source = readFile(AS_STRING(fname), &size);
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

	//init the new node
	callEngineNode(engine.rootNode, &engine.interpreter, "onInit");

	//cleanup
	freeLiteralArray(&inner.stack);
	freeLiteralArray(&inner.literalCache);
	freeLiteral(fname);

	return 0;
}

static int nativeLoadNode(Interpreter* interpreter, LiteralArray* arguments) {
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to loadNode\n");
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
		interpreter->errorOutput("Incorrect argument type passed to loadNode\n");
		freeLiteral(fname);
		return -1;
	}

	//load the new node
	size_t size = 0;
	char* source = readFile(AS_STRING(fname), &size);
	unsigned char* tb = compileString(source, &size);
	free((void*)source);

	EngineNode* node = ALLOCATE(EngineNode, 1);

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

	initEngineNode(node, &inner, tb, size);

	//NOTE: initNode() must be called manually

	// return the node
	Literal nodeLiteral = TO_OPAQUE_LITERAL(node, -1);
	pushLiteralArray(&interpreter->stack, nodeLiteral);

	//cleanup
	freeLiteralArray(&inner.stack);
	freeLiteralArray(&inner.literalCache);
	freeLiteral(fname);
	freeLiteral(nodeLiteral);

	return 1;
}

static int nativeInitNode(Interpreter* interpreter, LiteralArray* arguments) {
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to initNode\n");
		return -1;
	}

	Literal node = popLiteralArray(arguments);

	Literal nodeIdn = node;
	if (IS_IDENTIFIER(node) && parseIdentifierToValue(interpreter, &node)) {
		freeLiteral(nodeIdn);
	}

	//check argument types
	if (!IS_OPAQUE(node)) {
		interpreter->errorOutput("Incorrect argument type passed to initNode\n");
		freeLiteral(node);
		return -1;
	}

	EngineNode* engineNode = AS_OPAQUE(node);

	//init the new node
	callEngineNode(engineNode, &engine.interpreter, "onInit");

	//cleanup
	freeLiteral(node);
	return 0;
}

static int nativeFreeNode(Interpreter* interpreter, LiteralArray* arguments) {
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to freeNode\n");
		return -1;
	}

	Literal node = popLiteralArray(arguments);

	Literal nodeIdn = node;
	if (IS_IDENTIFIER(node) && parseIdentifierToValue(interpreter, &node)) {
		freeLiteral(nodeIdn);
	}

	//check argument types
	if (!IS_OPAQUE(node)) {
		interpreter->errorOutput("Incorrect argument type passed to freeNode\n");
		freeLiteral(node);
		return -1;
	}

	EngineNode* engineNode = AS_OPAQUE(node);

	//free the node
	callEngineNode(engineNode, &engine.interpreter, "onFree");
	freeEngineNode(engineNode);

	//cleanup
	freeLiteral(node);
	return 0;
}

static int nativeFreeChildNode(Interpreter* interpreter, LiteralArray* arguments) {
	if (arguments->count != 2) {
		interpreter->errorOutput("Incorrect number of arguments passed to freeChildNode\n");
		return -1;
	}

	Literal index = popLiteralArray(arguments);
	Literal node = popLiteralArray(arguments);

	Literal nodeIdn = node; //annoying
	if (IS_IDENTIFIER(node) && parseIdentifierToValue(interpreter, &node)) {
		freeLiteral(nodeIdn);
	}

	//check argument types
	if (!IS_OPAQUE(node) || !IS_INTEGER(index)) {
		interpreter->errorOutput("Incorrect argument type passed to freeChildNode\n");
		freeLiteral(node);
		return -1;
	}

	EngineNode* parentNode = AS_OPAQUE(node);
	int idx = AS_INTEGER(index);

	//check bounds
	if (idx < 0 || idx >= parentNode->count) {
		interpreter->errorOutput("Node index out of bounds in freeChildNode\n");
		freeLiteral(node);
		freeLiteral(index);
		return -1;
	}

	//get the child node
	EngineNode* childNode = parentNode->children[idx];

	//free the node
	if (childNode != NULL) {
		callEngineNode(childNode, &engine.interpreter, "onFree");
		freeEngineNode(childNode);
	}

	parentNode->children[idx] = NULL;

	//cleanup
	freeLiteral(node);
	freeLiteral(index);
	return 0;
}

static int nativePushNode(Interpreter* interpreter, LiteralArray* arguments) {
	//checks
	if (arguments->count != 2) {
		interpreter->errorOutput("Incorrect number of arguments passed to pushNode\n");
		return -1;
	}

	Literal child = popLiteralArray(arguments);
	Literal parent = popLiteralArray(arguments);

	Literal parentIdn = parent;
	if (IS_IDENTIFIER(parent) && parseIdentifierToValue(interpreter, &parent)) {
		freeLiteral(parentIdn);
	}

	Literal childIdn = child;
	if (IS_IDENTIFIER(child) && parseIdentifierToValue(interpreter, &child)) {
		freeLiteral(childIdn);
	}

	if (!IS_OPAQUE(parent) || !IS_OPAQUE(child)) {
		interpreter->errorOutput("Incorrect argument type passed to pushNode\n");
		freeLiteral(parent);
		freeLiteral(child);
		return -1;
	}

	//push the node
	EngineNode* parentNode = AS_OPAQUE(parent);
	EngineNode* childNode = AS_OPAQUE(child);

	pushEngineNode(parentNode, childNode);

	//no return value
	freeLiteral(parent);
	freeLiteral(child);

	return 0;
}

static int nativeGetNode(Interpreter* interpreter, LiteralArray* arguments) {
	//checks
	if (arguments->count != 2) {
		interpreter->errorOutput("Incorrect number of arguments passed to getNode\n");
		return -1;
	}

	Literal index = popLiteralArray(arguments);
	Literal parent = popLiteralArray(arguments);

	Literal parentIdn = parent;
	if (IS_IDENTIFIER(parent) && parseIdentifierToValue(interpreter, &parent)) {
		freeLiteral(parentIdn);
	}

	if (!IS_OPAQUE(parent) || !IS_INTEGER(index)) {
		interpreter->errorOutput("Incorrect argument type passed to getNode\n");
		freeLiteral(parent);
		freeLiteral(index);
		return -1;
	}

	//push the node
	EngineNode* parentNode = AS_OPAQUE(parent);
	int intIndex = AS_INTEGER(index);

	if (intIndex < 0 || intIndex >= parentNode->count) {
		interpreter->errorOutput("index out of bounds in getNode\n");
		freeLiteral(parent);
		freeLiteral(index);
		return -1;
	}

	EngineNode* childNode = parentNode->children[intIndex];
	Literal child = TO_OPAQUE_LITERAL(childNode, -1);

	pushLiteralArray(&interpreter->stack, child);

	//no return value
	freeLiteral(parent);
	freeLiteral(child);
	freeLiteral(index);

	return 1;
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
		{"loadNode", nativeLoadNode},
		{"_initNode", nativeInitNode},
		// {"freeNode", nativeFreeNode},
		{"_freeChildNode", nativeFreeChildNode},
		{"_pushNode", nativePushNode},
		{"_getNode", nativeGetNode},
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
