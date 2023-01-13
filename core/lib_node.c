#include "lib_node.h"

#include "engine.h"
#include "engine_node.h"
#include "repl_tools.h"

#include "memory.h"
#include "literal_array.h"

#include <stdlib.h>

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
	char* source = readFile(toCString(AS_STRING(fname)), &size);
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
	inner.hooks = interpreter->hooks;
	setInterpreterPrint(&inner, interpreter->printOutput);
	setInterpreterAssert(&inner, interpreter->assertOutput);
	setInterpreterError(&inner, interpreter->errorOutput);

	initEngineNode(node, &inner, tb, size);

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

	//init the new node (and ONLY this node)
	callEngineNode(engineNode, &engine.interpreter, "onInit", NULL);

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
		callRecursiveEngineNode(childNode, &engine.interpreter, "onFree", NULL);
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

static int nativeGetNodeChild(Interpreter* interpreter, LiteralArray* arguments) {
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
	Literal child = TO_OPAQUE_LITERAL(childNode, childNode->tag);

	pushLiteralArray(&interpreter->stack, child);

	//no return value
	freeLiteral(parent);
	freeLiteral(child);
	freeLiteral(index);

	return 1;
}

static int nativeGetNodeParent(Interpreter* interpreter, LiteralArray* arguments) {
	//checks
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to getNodeParent\n");
		return -1;
	}

	Literal nodeLiteral = popLiteralArray(arguments);

	Literal nodeIdn = nodeLiteral;
	if (IS_IDENTIFIER(nodeLiteral) && parseIdentifierToValue(interpreter, &nodeLiteral)) {
		freeLiteral(nodeIdn);
	}

	if (!IS_OPAQUE(nodeLiteral)) {
		interpreter->errorOutput("Incorrect argument type passed to getNodeParent\n");
		freeLiteral(nodeLiteral);
		return -1;
	}

	//push the node
	EngineNode* node = AS_OPAQUE(nodeLiteral);
	EngineNode* parent = node->parent;

	Literal parentLiteral = TO_NULL_LITERAL;
	if (parent != NULL) {
		parentLiteral = TO_OPAQUE_LITERAL(parent, parent->tag);
	}

	pushLiteralArray(&interpreter->stack, parentLiteral);

	//cleanup
	freeLiteral(parentLiteral);
	freeLiteral(nodeLiteral);

	return 1;
}

static int nativeLoadTexture(Interpreter* interpreter, LiteralArray* arguments) {
	if (arguments->count != 2) {
		interpreter->errorOutput("Incorrect number of arguments passed to loadTextureEngineNode\n");
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
		interpreter->errorOutput("Incorrect argument type passed to loadTextureEngineNode\n");
		freeLiteral(fname);
		freeLiteral(nodeLiteral);
		return -1;
	}

	//actually load TODO: number the opaques, and check the numbers
	EngineNode* node = (EngineNode*)AS_OPAQUE(nodeLiteral);

	if (node->texture != NULL) {
		freeTextureEngineNode(node);
	}

	if (loadTextureEngineNode(node, toCString(AS_STRING(fname))) != 0) {
		interpreter->errorOutput("Failed to load the texture into the EngineNode\n");
		freeLiteral(fname);
		freeLiteral(nodeLiteral);
		return -1;
	}

	//cleanup
	freeLiteral(fname);
	freeLiteral(nodeLiteral);
	return 0;
}

static int nativeFreeTexture(Interpreter* interpreter, LiteralArray* arguments) {
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to freeTextureEngineNode\n");
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
		interpreter->errorOutput("Incorrect argument type passed to freeTextureEngineNode\n");
		freeLiteral(nodeLiteral);
		return -1;
	}

	//actually load TODO: number the opaques, and check the numbers
	EngineNode* node = (EngineNode*)AS_OPAQUE(nodeLiteral);

	if (node->texture != NULL) {
		freeTextureEngineNode(node);
	}

	//cleanup
	freeLiteral(nodeLiteral);
	return 0;
}

static int nativeSetRect(Interpreter* interpreter, LiteralArray* arguments) {
	if (arguments->count != 5) {
		interpreter->errorOutput("Incorrect number of arguments passed to setRectEngineNode\n");
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
		interpreter->errorOutput("Incorrect argument type passed to setRectEngineNode\n");
		freeLiteral(nodeLiteral);
		freeLiteral(x);
		freeLiteral(y);
		freeLiteral(w);
		freeLiteral(h);
		return -1;
	}

	//actually set
	EngineNode* node = (EngineNode*)AS_OPAQUE(nodeLiteral);

	SDL_Rect r = {AS_INTEGER(x), AS_INTEGER(y), AS_INTEGER(w), AS_INTEGER(h)};
	setRectEngineNode(node, r);

	//cleanup
	freeLiteral(nodeLiteral);
	freeLiteral(x);
	freeLiteral(y);
	freeLiteral(w);
	freeLiteral(h);
	return 0;
}

//TODO: get x, y, w, h

static int nativeDrawNode(Interpreter* interpreter, LiteralArray* arguments) {
	if (arguments->count != 3 && arguments->count != 5) {
		interpreter->errorOutput("Incorrect number of arguments passed to drawEngineNode\n");
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
		interpreter->errorOutput("Incorrect argument type passed to drawEngineNode\n");
		freeLiteral(nodeLiteral);
		freeLiteral(x);
		freeLiteral(y);
		freeLiteral(w);
		freeLiteral(h);
		return -1;
	}

	//actually render
	EngineNode* node = (EngineNode*)AS_OPAQUE(nodeLiteral);

	SDL_Rect r = {AS_INTEGER(x), AS_INTEGER(y), 0, 0};
	if (IS_INTEGER(w) && IS_INTEGER(h)) {
		r.w = AS_INTEGER(w);
		r.h = AS_INTEGER(h);
	}
	else {
		r.w = node->rect.w;
		r.h = node->rect.h;
	}

	drawEngineNode(node, r);

	//cleanup
	freeLiteral(nodeLiteral);
	freeLiteral(x);
	freeLiteral(y);
	freeLiteral(w);
	freeLiteral(h);
	return 0;
}

static int nativeGetNodeTag(Interpreter* interpreter, LiteralArray* arguments) {
	//checks
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to getNodeTag\n");
		return -1;
	}

	Literal nodeLiteral = popLiteralArray(arguments);

	Literal nodeIdn = nodeLiteral;
	if (IS_IDENTIFIER(nodeLiteral) && parseIdentifierToValue(interpreter, &nodeLiteral)) {
		freeLiteral(nodeIdn);
	}

	if (!IS_OPAQUE(nodeLiteral)) {
		interpreter->errorOutput("Incorrect argument type passed to getNodeTag\n");
		freeLiteral(nodeLiteral);
		return -1;
	}

	//push the tag
	Literal tagLiteral = TO_INTEGER_LITERAL( ((EngineNode*)AS_OPAQUE(nodeLiteral))->tag );

	pushLiteralArray(&interpreter->stack, tagLiteral);

	//cleanup
	freeLiteral(nodeLiteral);
	freeLiteral(tagLiteral);

	return 1;
}

static int nativeCallNode(Interpreter* interpreter, LiteralArray* arguments) {
	//checks
	if (arguments->count < 2) {
		interpreter->errorOutput("Too few arguments passed to callEngineNode\n");
		return -1;
	}

	LiteralArray extraArgs;
	initLiteralArray(&extraArgs);

	LiteralArray flippedExtraArgs;
	initLiteralArray(&flippedExtraArgs);

	//extract the extra arg values
	while (arguments->count > 2) {
		Literal tmp = popLiteralArray(arguments);

		Literal idn = tmp; //there's almost certainly a better way of doing all of this stuff
		if (IS_IDENTIFIER(tmp) && parseIdentifierToValue(interpreter, &tmp)) {
			freeLiteral(idn);
		}

		pushLiteralArray(&flippedExtraArgs, tmp);
		freeLiteral(tmp);
	}

	//correct the order
	while (flippedExtraArgs.count) {
		Literal tmp = popLiteralArray(&flippedExtraArgs);
		pushLiteralArray(&extraArgs, tmp);
		freeLiteral(tmp);
	}

	freeLiteralArray(&flippedExtraArgs);

	//back on track
	Literal fnName = popLiteralArray(arguments);
	Literal nodeLiteral = popLiteralArray(arguments);

	Literal nodeIdn = nodeLiteral;
	if (IS_IDENTIFIER(nodeLiteral) && parseIdentifierToValue(interpreter, &nodeLiteral)) {
		freeLiteral(nodeIdn);
	}

	Literal fnNameIdn = fnName;
	if (IS_IDENTIFIER(fnName) && parseIdentifierToValue(interpreter, &fnName)) {
		freeLiteral(fnNameIdn);
	}

	if (!IS_OPAQUE(nodeLiteral) || !IS_STRING(fnName)) {
		interpreter->errorOutput("Incorrect argument type passed to callEngineNode\n");
		freeLiteral(nodeLiteral);
		freeLiteral(fnName);
		return -1;
	}

	//allow refstring to do it's magic
	Literal fnNameIdentifier = TO_IDENTIFIER_LITERAL(copyRefString(AS_STRING(fnName)));

	//call the function
	Literal result = callEngineNodeLiteral(AS_OPAQUE(nodeLiteral), interpreter, fnNameIdentifier, &extraArgs);

	pushLiteralArray(&interpreter->stack, result);

	//cleanup
	freeLiteralArray(&extraArgs);
	freeLiteral(nodeLiteral);
	freeLiteral(fnName);
	freeLiteral(result);

	return 1;
}

//call the hook
typedef struct Natives {
	char* name;
	NativeFn fn;
} Natives;

int hookNode(Interpreter* interpreter, Literal identifier, Literal alias) {
	//build the natives list
	Natives natives[] = {
		{"loadNode", nativeLoadNode},
		{"_initNode", nativeInitNode},
		{"_freeChildNode", nativeFreeChildNode},
		{"_pushNode", nativePushNode},
		{"_getNodeChild", nativeGetNodeChild},
		{"_getNodeParent", nativeGetNodeParent},
		{"_loadTexture", nativeLoadTexture},
		{"_freeTexture", nativeFreeTexture},
		{"_setRect", nativeSetRect},
		{"_drawNode", nativeDrawNode},
		{"_callNode", nativeCallNode},
		// {"getNodeTag", nativeGetNodeTag}, //not needed if there's only one node type
		{NULL, NULL},
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
