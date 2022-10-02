#include "lib_engine.h"

#include "engine.h"

#include "memory.h"
#include "literal_array.h"

//errors here should be fatal
static void fatalError(char* message) {
	fprintf(stderr, message);
	exit(-1);
}

//compilation functions
//TODO: move these to their own file
#include "console_colors.h"
#include "lexer.h"
#include "parser.h"
#include "compiler.h"
#include "interpreter.h"

static char* readFile(char* path, size_t* fileSize) {
	FILE* file = fopen(path, "rb");

	if (file == NULL) {
		fprintf(stderr, ERROR "Could not open file \"%s\"\n" RESET, path);
		exit(-1);
	}

	fseek(file, 0L, SEEK_END);
	*fileSize = ftell(file);
	rewind(file);

	char* buffer = (char*)malloc(*fileSize + 1);

	if (buffer == NULL) {
		fprintf(stderr, ERROR "Not enough memory to read \"%s\"\n" RESET, path);
		exit(-1);
	}

	size_t bytesRead = fread(buffer, sizeof(char), *fileSize, file);

	buffer[*fileSize] = '\0'; //NOTE: fread doesn't append this

	if (bytesRead < *fileSize) {
		fprintf(stderr, ERROR "Could not read file \"%s\"\n" RESET, path);
		exit(-1);
	}

	fclose(file);

	return buffer;
}

static unsigned char* compileString(char* source, size_t* size) {
	Lexer lexer;
	Parser parser;
	Compiler compiler;

	initLexer(&lexer, source);
	initParser(&parser, &lexer);
	initCompiler(&compiler);

	//run the parser until the end of the source
	ASTNode* node = scanParser(&parser);
	while(node != NULL) {
		//pack up and leave
		if (node->type == AST_NODEERROR) {
			printf(ERROR "error node detected\n" RESET);
			freeNode(node);
			freeCompiler(&compiler);
			freeParser(&parser);
			return NULL;
		}

		writeCompiler(&compiler, node);
		freeNode(node);
		node = scanParser(&parser);
	}

	//get the bytecode dump
	unsigned char* tb = collateCompiler(&compiler, (int*)(size));

	//cleanup
	freeCompiler(&compiler);
	freeParser(&parser);
	//no lexer to clean up

	//finally
	return tb;
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
	engine.renderer = SDL_CreateRenderer(engine.window, -1, 0);

	if (engine.renderer == NULL) {
		fatalError("Failed to initialize the renderer\n");
	}

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

static int nativeLoadRootNode(Interpreter* interpreter, LiteralArray* arguments) {
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to loadRootNode\n");
		return -1;
	}

	//extract the arguments
	Literal fname = popLiteralArray(arguments);

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

	//BUGFIX: use an inner-interpreter here, otherwise it'll mess up the original's length value by calling run within a native function
	Interpreter inner;
	initInterpreter(&inner);

	initEngineNode(engine.rootNode, &inner, tb, size);

	freeInterpreter(&inner);

	//init the new node
	callEngineNode(engine.rootNode, &engine.interpreter, "onInit");

	//cleanup
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
