#include "lib_render.h"
#include "render_node.h"

#include "engine.h"
#include "repl_tools.h"

#include "memory.h"
#include "literal_array.h"

static int nativeLoadRenderNode(Interpreter* interpreter, LiteralArray* arguments) {
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to loadRenderNode\n");
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
		interpreter->errorOutput("Incorrect argument type passed to loadRenderNode\n");
		freeLiteral(fname);
		return -1;
	}

	//load the new node
	size_t size = 0;
	char* source = readFile(AS_STRING(fname), &size);
	unsigned char* tb = compileString(source, &size);
	free((void*)source);

	RenderNode* node = ALLOCATE(RenderNode, 1);

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

	initRenderNode(node, &inner, tb, size);

	//NOTE: initNode() must be called manually

	// return the node
	Literal nodeLiteral = TO_OPAQUE_LITERAL(node, node->tag);
	pushLiteralArray(&interpreter->stack, nodeLiteral);

	//cleanup
	freeLiteralArray(&inner.stack);
	freeLiteralArray(&inner.literalCache);
	freeLiteral(fname);
	freeLiteral(nodeLiteral);

	return 1;
}

static int nativeLoadTextureRenderNode(Interpreter* interpreter, LiteralArray* arguments) {
	if (arguments->count != 2) {
		interpreter->errorOutput("Incorrect number of arguments passed to loadTextureRenderNode\n");
		return -1;
	}

	//extract the arguments
	Literal fname = popLiteralArray(arguments);
	Literal nodeLiteral = popLiteralArray(arguments);

	Literal fnameIdn = fname;
	if (IS_IDENTIFIER(fname) && parseIdentifierToValue(interpreter, &fname)) {
		freeLiteral(fnameIdn);
	}

	Literal nodeIdn = nodeLiteral;
	if (IS_IDENTIFIER(nodeLiteral) && parseIdentifierToValue(interpreter, &nodeLiteral)) {
		freeLiteral(nodeIdn);
	}

	//check argument types
	if (!IS_STRING(fname) || !IS_OPAQUE(nodeLiteral)) {
		interpreter->errorOutput("Incorrect argument type passed to loadTextureRenderNode\n");
		freeLiteral(fname);
		freeLiteral(nodeLiteral);
		return -1;
	}

	//actually load TODO: number the opaques, and check the numbers
	RenderNode* node = (RenderNode*)AS_OPAQUE(nodeLiteral);

	if (node->texture != NULL) {
		freeTextureRenderNode(node);
	}

	if (loadTextureRenderNode(node, AS_STRING(fname)) != 0) {
		interpreter->errorOutput("Failed to load the texture into the RenderNode\n");
		freeLiteral(fname);
		freeLiteral(nodeLiteral);
		return -1;
	}

	//cleanup
	freeLiteral(fname);
	freeLiteral(nodeLiteral);
	return 0;
}

static int nativeFreeTextureRenderNode(Interpreter* interpreter, LiteralArray* arguments) {
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to freeTextureRenderNode\n");
		return -1;
	}

	//extract the arguments
	Literal nodeLiteral = popLiteralArray(arguments);

	Literal nodeIdn = nodeLiteral;
	if (IS_IDENTIFIER(nodeLiteral) && parseIdentifierToValue(interpreter, &nodeLiteral)) {
		freeLiteral(nodeIdn);
	}

	//check argument types
	if (!IS_OPAQUE(nodeLiteral)) {
		interpreter->errorOutput("Incorrect argument type passed to freeTextureRenderNode\n");
		freeLiteral(nodeLiteral);
		return -1;
	}

	//actually load TODO: number the opaques, and check the numbers
	RenderNode* node = (RenderNode*)AS_OPAQUE(nodeLiteral);

	if (node->texture != NULL) {
		freeTextureRenderNode(node);
	}

	//cleanup
	freeLiteral(nodeLiteral);
	return 0;
}

static int nativeSetRectRenderNode(Interpreter* interpreter, LiteralArray* arguments) {
	if (arguments->count != 5) {
		interpreter->errorOutput("Incorrect number of arguments passed to setRectRenderNode\n");
		return -1;
	}

	//extract the arguments
	Literal h = popLiteralArray(arguments);
	Literal w = popLiteralArray(arguments);
	Literal y = popLiteralArray(arguments);
	Literal x = popLiteralArray(arguments);
	Literal nodeLiteral = popLiteralArray(arguments);

	Literal nodeIdn = nodeLiteral;
	if (IS_IDENTIFIER(nodeLiteral) && parseIdentifierToValue(interpreter, &nodeLiteral)) {
		freeLiteral(nodeIdn);
	}

	Literal xi = x;
	if (IS_IDENTIFIER(x) && parseIdentifierToValue(interpreter, &x)) {
		freeLiteral(xi);
	}

	Literal yi = y;
	if (IS_IDENTIFIER(y) && parseIdentifierToValue(interpreter, &y)) {
		freeLiteral(yi);
	}

	Literal wi = w;
	if (IS_IDENTIFIER(w) && parseIdentifierToValue(interpreter, &w)) {
		freeLiteral(wi);
	}

	Literal hi = h;
	if (IS_IDENTIFIER(h) && parseIdentifierToValue(interpreter, &h)) {
		freeLiteral(hi);
	}

	//check argument types
	if (!IS_OPAQUE(nodeLiteral) || !IS_INTEGER(x) || !IS_INTEGER(y) || !IS_INTEGER(w) || !IS_INTEGER(h)) {
		interpreter->errorOutput("Incorrect argument type passed to setRectRenderNode\n");
		freeLiteral(nodeLiteral);
		freeLiteral(xi);
		freeLiteral(yi);
		freeLiteral(wi);
		freeLiteral(hi);
		return -1;
	}

	//actually set
	RenderNode* node = (RenderNode*)AS_OPAQUE(nodeLiteral);

	SDL_Rect r = {AS_INTEGER(x), AS_INTEGER(y), AS_INTEGER(w), AS_INTEGER(h)};
	setRectRenderNode(node, r);

	//cleanup
	freeLiteral(nodeLiteral);
	freeLiteral(xi);
	freeLiteral(yi);
	freeLiteral(wi);
	freeLiteral(hi);
	return 0;
}

static int nativeDrawRenderNode(Interpreter* interpreter, LiteralArray* arguments) {
	if (arguments->count != 3 && arguments->count != 5) {
		interpreter->errorOutput("Incorrect number of arguments passed to drawRenderNode\n");
		return -1;
	}

	//extract the arguments
	Literal w = TO_NULL_LITERAL, h = TO_NULL_LITERAL;
	if (arguments->count == 5) {
		h = popLiteralArray(arguments);
		w = popLiteralArray(arguments);
	}

	Literal y = popLiteralArray(arguments);
	Literal x = popLiteralArray(arguments);
	Literal nodeLiteral = popLiteralArray(arguments);

	Literal nodeIdn = nodeLiteral;
	if (IS_IDENTIFIER(nodeLiteral) && parseIdentifierToValue(interpreter, &nodeLiteral)) {
		freeLiteral(nodeIdn);
	}

	Literal xi = x;
	if (IS_IDENTIFIER(x) && parseIdentifierToValue(interpreter, &x)) {
		freeLiteral(xi);
	}

	Literal yi = y;
	if (IS_IDENTIFIER(y) && parseIdentifierToValue(interpreter, &y)) {
		freeLiteral(yi);
	}

	Literal wi = w;
	if (IS_IDENTIFIER(w) && parseIdentifierToValue(interpreter, &w)) {
		freeLiteral(wi);
	}

	Literal hi = h;
	if (IS_IDENTIFIER(h) && parseIdentifierToValue(interpreter, &h)) {
		freeLiteral(hi);
	}

	//check argument types
	if (!IS_OPAQUE(nodeLiteral) || !IS_INTEGER(x) || !IS_INTEGER(y) || (!IS_INTEGER(w) && !IS_NULL(w)) || (!IS_INTEGER(h) && !IS_NULL(h))) {
		interpreter->errorOutput("Incorrect argument type passed to drawRenderNode\n");
		freeLiteral(nodeLiteral);
		freeLiteral(xi);
		freeLiteral(yi);
		freeLiteral(wi);
		freeLiteral(hi);
		return -1;
	}

	//actually render
	RenderNode* node = (RenderNode*)AS_OPAQUE(nodeLiteral);

	SDL_Rect r = {AS_INTEGER(x), AS_INTEGER(y), 0, 0};
	if (IS_INTEGER(w) && IS_INTEGER(h)) {
		r.w = AS_INTEGER(w);
		r.h = AS_INTEGER(h);
	}
	else {
		r.w = node->rect.w;
		r.h = node->rect.h;
	}

	drawRenderNode(node, r);

	//cleanup
	freeLiteral(nodeLiteral);
	freeLiteral(xi);
	freeLiteral(yi);
	freeLiteral(wi);
	freeLiteral(hi);
	return 0;
}

//call the hook
typedef struct Natives {
	char* name;
	NativeFn fn;
} Natives;

int hookRender(Interpreter* interpreter, Literal identifier, Literal alias) {
	//build the natives list
	Natives natives[] = {
		{"loadRenderNode", nativeLoadRenderNode},
		{"_loadTextureRenderNode", nativeLoadTextureRenderNode},
		{"_freeTextureRenderNode", nativeFreeTextureRenderNode},
		{"_setRectRenderNode", nativeSetRectRenderNode},
		{"_drawRenderNode", nativeDrawRenderNode},
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
