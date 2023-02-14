#include "lib_node.h"

#include "box_engine_node.h"
#include "box_engine.h"

#include "repl_tools.h"
#include "toy_literal_array.h"
#include "toy_memory.h"

#include "lib_runner.h"

#include <stdlib.h>

static int nativeLoadNode(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to loadNode\n");
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
		interpreter->errorOutput("Incorrect argument type passed to loadNode\n");
		Toy_freeLiteral(drivePathLiteral);
		return -1;
	}

	Toy_Literal filePathLiteral = Toy_getFilePathLiteral(interpreter, &drivePathLiteral);

	if (!TOY_IS_STRING(filePathLiteral)) {
		Toy_freeLiteral(drivePathLiteral);
		Toy_freeLiteral(filePathLiteral);
		return -1;
	}

	Toy_freeLiteral(drivePathLiteral); //not needed anymore

	//load the new node
	size_t size = 0;
	const char* source = Toy_readFile(Toy_toCString(TOY_AS_STRING(filePathLiteral)), &size);
	const unsigned char* tb = Toy_compileString(source, &size);
	free((void*)source);

	Box_EngineNode* node = TOY_ALLOCATE(Box_EngineNode, 1);

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

	Box_initEngineNode(node, &inner, tb, size);

	// return the node
	Toy_Literal nodeLiteral = TOY_TO_OPAQUE_LITERAL(node, node->tag);
	Toy_pushLiteralArray(&interpreter->stack, nodeLiteral);

	//cleanup
	while (inner.scope) {
		inner.scope = Toy_popScope(inner.scope);
	}

	Toy_freeLiteralArray(&inner.stack);
	Toy_freeLiteralArray(&inner.literalCache);
	Toy_freeLiteral(filePathLiteral);
	Toy_freeLiteral(nodeLiteral);

	return 1;
}

static int nativeInitNode(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to initNode\n");
		return -1;
	}

	Toy_Literal node = Toy_popLiteralArray(arguments);

	Toy_Literal nodeIdn = node;
	if (TOY_IS_IDENTIFIER(node) && Toy_parseIdentifierToValue(interpreter, &node)) {
		Toy_freeLiteral(nodeIdn);
	}

	//check argument types
	if (!TOY_IS_OPAQUE(node)) {
		interpreter->errorOutput("Incorrect argument type passed to initNode\n");
		Toy_freeLiteral(node);
		return -1;
	}

	Box_EngineNode* engineNode = TOY_AS_OPAQUE(node);

	//init the new node (and ONLY this node)
	Box_callEngineNode(engineNode, &engine.interpreter, "onInit", NULL);

	//cleanup
	Toy_freeLiteral(node);
	return 0;
}

static int nativeFreeChildNode(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	if (arguments->count != 2) {
		interpreter->errorOutput("Incorrect number of arguments passed to freeChildNode\n");
		return -1;
	}

	Toy_Literal index = Toy_popLiteralArray(arguments);
	Toy_Literal node = Toy_popLiteralArray(arguments);

	Toy_Literal nodeIdn = node; //annoying
	if (TOY_IS_IDENTIFIER(node) && Toy_parseIdentifierToValue(interpreter, &node)) {
		Toy_freeLiteral(nodeIdn);
	}

	//check argument types
	if (!TOY_IS_OPAQUE(node) || !TOY_IS_INTEGER(index)) {
		interpreter->errorOutput("Incorrect argument type passed to freeChildNode\n");
		Toy_freeLiteral(node);
		return -1;
	}

	Box_EngineNode* parentNode = TOY_AS_OPAQUE(node);
	int idx = TOY_AS_INTEGER(index);

	//check bounds
	if (idx < 0 || idx >= parentNode->count) {
		interpreter->errorOutput("Node index out of bounds in freeChildNode\n");
		Toy_freeLiteral(node);
		Toy_freeLiteral(index);
		return -1;
	}

	//get the child node
	Box_EngineNode* childNode = parentNode->children[idx];

	//free the node
	if (childNode != NULL) {
		Box_callRecursiveEngineNode(childNode, &engine.interpreter, "onFree", NULL);
		Box_freeEngineNode(childNode);
	}

	parentNode->children[idx] = NULL;

	//cleanup
	Toy_freeLiteral(node);
	Toy_freeLiteral(index);
	return 0;
}

static int nativePushNode(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	//checks
	if (arguments->count != 2) {
		interpreter->errorOutput("Incorrect number of arguments passed to pushNode\n");
		return -1;
	}

	Toy_Literal child = Toy_popLiteralArray(arguments);
	Toy_Literal parent = Toy_popLiteralArray(arguments);

	Toy_Literal parentIdn = parent;
	if (TOY_IS_IDENTIFIER(parent) && Toy_parseIdentifierToValue(interpreter, &parent)) {
		Toy_freeLiteral(parentIdn);
	}

	Toy_Literal childIdn = child;
	if (TOY_IS_IDENTIFIER(child) && Toy_parseIdentifierToValue(interpreter, &child)) {
		Toy_freeLiteral(childIdn);
	}

	if (!TOY_IS_OPAQUE(parent) || !TOY_IS_OPAQUE(child)) {
		interpreter->errorOutput("Incorrect argument type passed to pushNode\n");
		Toy_freeLiteral(parent);
		Toy_freeLiteral(child);
		return -1;
	}

	//push the node
	Box_EngineNode* parentNode = TOY_AS_OPAQUE(parent);
	Box_EngineNode* childNode = TOY_AS_OPAQUE(child);

	Box_pushEngineNode(parentNode, childNode);

	//no return value
	Toy_freeLiteral(parent);
	Toy_freeLiteral(child);

	return 0;
}

static int nativeGetNodeChild(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	//checks
	if (arguments->count != 2) {
		interpreter->errorOutput("Incorrect number of arguments passed to getNodeChild\n");
		return -1;
	}

	Toy_Literal index = Toy_popLiteralArray(arguments);
	Toy_Literal parent = Toy_popLiteralArray(arguments);

	Toy_Literal parentIdn = parent;
	if (TOY_IS_IDENTIFIER(parent) && Toy_parseIdentifierToValue(interpreter, &parent)) {
		Toy_freeLiteral(parentIdn);
	}

	if (!TOY_IS_OPAQUE(parent) || !TOY_IS_INTEGER(index)) {
		interpreter->errorOutput("Incorrect argument type passed to getNodeChild\n");
		Toy_freeLiteral(parent);
		Toy_freeLiteral(index);
		return -1;
	}

	//push the node
	Box_EngineNode* parentNode = TOY_AS_OPAQUE(parent);
	int intIndex = TOY_AS_INTEGER(index);

	if (intIndex < 0 || intIndex >= parentNode->count) {
		interpreter->errorOutput("index out of bounds in getNodeChild\n");
		Toy_freeLiteral(parent);
		Toy_freeLiteral(index);
		return -1;
	}

	Box_EngineNode* childNode = parentNode->children[intIndex];
	Toy_Literal child = TOY_TO_OPAQUE_LITERAL(childNode, childNode->tag);

	Toy_pushLiteralArray(&interpreter->stack, child);

	//no return value
	Toy_freeLiteral(parent);
	Toy_freeLiteral(child);
	Toy_freeLiteral(index);

	return 1;
}

static int nativeGetNodeParent(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	//checks
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to getNodeParent\n");
		return -1;
	}

	Toy_Literal nodeLiteral = Toy_popLiteralArray(arguments);

	Toy_Literal nodeIdn = nodeLiteral;
	if (TOY_IS_IDENTIFIER(nodeLiteral) && Toy_parseIdentifierToValue(interpreter, &nodeLiteral)) {
		Toy_freeLiteral(nodeIdn);
	}

	if (!TOY_IS_OPAQUE(nodeLiteral)) {
		interpreter->errorOutput("Incorrect argument type passed to getNodeParent\n");
		Toy_freeLiteral(nodeLiteral);
		return -1;
	}

	//push the node
	Box_EngineNode* node = TOY_AS_OPAQUE(nodeLiteral);
	Box_EngineNode* parent = node->parent;

	Toy_Literal parentLiteral = TOY_TO_NULL_LITERAL;
	if (parent != NULL) {
		parentLiteral = TOY_TO_OPAQUE_LITERAL(parent, parent->tag);
	}

	Toy_pushLiteralArray(&interpreter->stack, parentLiteral);

	//cleanup
	Toy_freeLiteral(parentLiteral);
	Toy_freeLiteral(nodeLiteral);

	return 1;
}

static int nativeLoadTexture(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	if (arguments->count != 2) {
		interpreter->errorOutput("Incorrect number of arguments passed to loadTexture\n");
		return -1;
	}

	//extract the arguments
	Toy_Literal drivePathLiteral = Toy_popLiteralArray(arguments);
	Toy_Literal nodeLiteral = Toy_popLiteralArray(arguments);

	Toy_Literal drivePathLiteralIdn = drivePathLiteral;
	if (TOY_IS_IDENTIFIER(drivePathLiteral) && Toy_parseIdentifierToValue(interpreter, &drivePathLiteral)) {
		Toy_freeLiteral(drivePathLiteralIdn);
	}

	Toy_Literal nodeIdn = nodeLiteral;
	if (TOY_IS_IDENTIFIER(nodeLiteral) && Toy_parseIdentifierToValue(interpreter, &nodeLiteral)) {
		Toy_freeLiteral(nodeIdn);
	}

	//check argument types
	if (!TOY_IS_STRING(drivePathLiteral) || !TOY_IS_OPAQUE(nodeLiteral)) {
		interpreter->errorOutput("Incorrect argument type passed to loadTexture\n");
		Toy_freeLiteral(drivePathLiteral);
		Toy_freeLiteral(nodeLiteral);
		return -1;
	}

	Toy_Literal filePathLiteral = Toy_getFilePathLiteral(interpreter, &drivePathLiteral);

	if (!TOY_IS_STRING(filePathLiteral)) {
		Toy_freeLiteral(drivePathLiteral);
		Toy_freeLiteral(filePathLiteral);
		return -1;
	}

	Toy_freeLiteral(drivePathLiteral); //not needed anymore

	//actually load TODO: number the opaques, and check the numbers
	Box_EngineNode* node = (Box_EngineNode*)TOY_AS_OPAQUE(nodeLiteral);

	if (node->texture != NULL) {
		Box_freeTextureEngineNode(node);
	}

	if (Box_loadTextureEngineNode(node, Toy_toCString(TOY_AS_STRING(filePathLiteral))) != 0) {
		interpreter->errorOutput("Failed to load the texture into the EngineNode\n");
		Toy_freeLiteral(filePathLiteral);
		Toy_freeLiteral(nodeLiteral);
		return -1;
	}

	//cleanup
	Toy_freeLiteral(filePathLiteral);
	Toy_freeLiteral(nodeLiteral);

	return 0;
}

static int nativeFreeTexture(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to freeTexture\n");
		return -1;
	}

	//extract the arguments
	Toy_Literal nodeLiteral = Toy_popLiteralArray(arguments);

	Toy_Literal nodeIdn = nodeLiteral;
	if (TOY_IS_IDENTIFIER(nodeLiteral) && Toy_parseIdentifierToValue(interpreter, &nodeLiteral)) {
		Toy_freeLiteral(nodeIdn);
	}

	//check argument types
	if (!TOY_IS_OPAQUE(nodeLiteral)) {
		interpreter->errorOutput("Incorrect argument type passed to freeTexture\n");
		Toy_freeLiteral(nodeLiteral);
		return -1;
	}

	//actually load TODO: number the opaques, and check the numbers
	Box_EngineNode* node = (Box_EngineNode*)TOY_AS_OPAQUE(nodeLiteral);

	if (node->texture != NULL) {
		Box_freeTextureEngineNode(node);
	}

	//cleanup
	Toy_freeLiteral(nodeLiteral);

	return 0;
}

static int nativeSetRect(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	if (arguments->count != 5) {
		interpreter->errorOutput("Incorrect number of arguments passed to setRect\n");
		return -1;
	}

	//extract the arguments
	Toy_Literal h = Toy_popLiteralArray(arguments);
	Toy_Literal w = Toy_popLiteralArray(arguments);
	Toy_Literal y = Toy_popLiteralArray(arguments);
	Toy_Literal x = Toy_popLiteralArray(arguments);
	Toy_Literal nodeLiteral = Toy_popLiteralArray(arguments);

	Toy_Literal nodeIdn = nodeLiteral;
	if (TOY_IS_IDENTIFIER(nodeLiteral) && Toy_parseIdentifierToValue(interpreter, &nodeLiteral)) {
		Toy_freeLiteral(nodeIdn);
	}

	Toy_Literal xi = x;
	if (TOY_IS_IDENTIFIER(x) && Toy_parseIdentifierToValue(interpreter, &x)) {
		Toy_freeLiteral(xi);
	}

	Toy_Literal yi = y;
	if (TOY_IS_IDENTIFIER(y) && Toy_parseIdentifierToValue(interpreter, &y)) {
		Toy_freeLiteral(yi);
	}

	Toy_Literal wi = w;
	if (TOY_IS_IDENTIFIER(w) && Toy_parseIdentifierToValue(interpreter, &w)) {
		Toy_freeLiteral(wi);
	}

	Toy_Literal hi = h;
	if (TOY_IS_IDENTIFIER(h) && Toy_parseIdentifierToValue(interpreter, &h)) {
		Toy_freeLiteral(hi);
	}

	//check argument types
	if (!TOY_IS_OPAQUE(nodeLiteral) || !TOY_IS_INTEGER(x) || !TOY_IS_INTEGER(y) || !TOY_IS_INTEGER(w) || !TOY_IS_INTEGER(h)) {
		interpreter->errorOutput("Incorrect argument type passed to setRect\n");
		Toy_freeLiteral(nodeLiteral);
		Toy_freeLiteral(x);
		Toy_freeLiteral(y);
		Toy_freeLiteral(w);
		Toy_freeLiteral(h);
		return -1;
	}

	//actually set
	Box_EngineNode* node = (Box_EngineNode*)TOY_AS_OPAQUE(nodeLiteral);

	SDL_Rect r = {TOY_AS_INTEGER(x), TOY_AS_INTEGER(y), TOY_AS_INTEGER(w), TOY_AS_INTEGER(h)};
	Box_setRectEngineNode(node, r);

	//cleanup
	Toy_freeLiteral(nodeLiteral);
	Toy_freeLiteral(x);
	Toy_freeLiteral(y);
	Toy_freeLiteral(w);
	Toy_freeLiteral(h);

	return 0;
}

//TODO: get x, y, w, h

static int nativeDrawNode(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	if (arguments->count != 3 && arguments->count != 5) {
		interpreter->errorOutput("Incorrect number of arguments passed to drawNode\n");
		return -1;
	}

	//extract the arguments
	Toy_Literal w = TOY_TO_NULL_LITERAL, h = TOY_TO_NULL_LITERAL;
	if (arguments->count == 5) {
		h = Toy_popLiteralArray(arguments);
		w = Toy_popLiteralArray(arguments);
	}

	Toy_Literal y = Toy_popLiteralArray(arguments);
	Toy_Literal x = Toy_popLiteralArray(arguments);
	Toy_Literal nodeLiteral = Toy_popLiteralArray(arguments);

	Toy_Literal nodeIdn = nodeLiteral;
	if (TOY_IS_IDENTIFIER(nodeLiteral) && Toy_parseIdentifierToValue(interpreter, &nodeLiteral)) {
		Toy_freeLiteral(nodeIdn);
	}

	Toy_Literal xi = x;
	if (TOY_IS_IDENTIFIER(x) && Toy_parseIdentifierToValue(interpreter, &x)) {
		Toy_freeLiteral(xi);
	}

	Toy_Literal yi = y;
	if (TOY_IS_IDENTIFIER(y) && Toy_parseIdentifierToValue(interpreter, &y)) {
		Toy_freeLiteral(yi);
	}

	Toy_Literal wi = w;
	if (TOY_IS_IDENTIFIER(w) && Toy_parseIdentifierToValue(interpreter, &w)) {
		Toy_freeLiteral(wi);
	}

	Toy_Literal hi = h;
	if (TOY_IS_IDENTIFIER(h) && Toy_parseIdentifierToValue(interpreter, &h)) {
		Toy_freeLiteral(hi);
	}

	//check argument types
	if (!TOY_IS_OPAQUE(nodeLiteral) || !TOY_IS_INTEGER(x) || !TOY_IS_INTEGER(y) || (!TOY_IS_INTEGER(w) && !TOY_IS_NULL(w)) || (!TOY_IS_INTEGER(h) && !TOY_IS_NULL(h))) {
		interpreter->errorOutput("Incorrect argument type passed to drawNode\n");
		Toy_freeLiteral(nodeLiteral);
		Toy_freeLiteral(x);
		Toy_freeLiteral(y);
		Toy_freeLiteral(w);
		Toy_freeLiteral(h);
		return -1;
	}

	//actually render
	Box_EngineNode* node = (Box_EngineNode*)TOY_AS_OPAQUE(nodeLiteral);

	SDL_Rect r = {TOY_AS_INTEGER(x), TOY_AS_INTEGER(y), 0, 0};
	if (TOY_IS_INTEGER(w) && TOY_IS_INTEGER(h)) {
		r.w = TOY_AS_INTEGER(w);
		r.h = TOY_AS_INTEGER(h);
	}
	else {
		r.w = node->rect.w;
		r.h = node->rect.h;
	}

	Box_drawEngineNode(node, r);

	//cleanup
	Toy_freeLiteral(nodeLiteral);
	Toy_freeLiteral(x);
	Toy_freeLiteral(y);
	Toy_freeLiteral(w);
	Toy_freeLiteral(h);

	return 0;
}

static int nativeCallNode(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	//checks
	if (arguments->count < 2) {
		interpreter->errorOutput("Too few arguments passed to callNode\n");
		return -1;
	}

	Toy_LiteralArray extraArgs;
	Toy_initLiteralArray(&extraArgs);

	//extract the extra arg values
	while (arguments->count > 2) {
		Toy_Literal tmp = Toy_popLiteralArray(arguments);

		Toy_Literal idn = tmp; //there's almost certainly a better way of doing all of this stuff
		if (TOY_IS_IDENTIFIER(tmp) && Toy_parseIdentifierToValue(interpreter, &tmp)) {
			Toy_freeLiteral(idn);
		}

		Toy_pushLiteralArray(&extraArgs, tmp);
		Toy_freeLiteral(tmp);
	}

	//back on track
	Toy_Literal fnName = Toy_popLiteralArray(arguments);
	Toy_Literal nodeLiteral = Toy_popLiteralArray(arguments);

	Toy_Literal nodeIdn = nodeLiteral;
	if (TOY_IS_IDENTIFIER(nodeLiteral) && Toy_parseIdentifierToValue(interpreter, &nodeLiteral)) {
		Toy_freeLiteral(nodeIdn);
	}

	Toy_Literal fnNameIdn = fnName;
	if (TOY_IS_IDENTIFIER(fnName) && Toy_parseIdentifierToValue(interpreter, &fnName)) {
		Toy_freeLiteral(fnNameIdn);
	}

	if (!TOY_IS_OPAQUE(nodeLiteral) || !TOY_IS_STRING(fnName)) {
		interpreter->errorOutput("Incorrect argument type passed to callNode\n");
		Toy_freeLiteral(nodeLiteral);
		Toy_freeLiteral(fnName);
		return -1;
	}

	//allow refstring to do it's magic
	Toy_Literal fnNameIdentifier = TOY_TO_IDENTIFIER_LITERAL(Toy_copyRefString(TOY_AS_STRING(fnName)));

	//call the function
	Toy_Literal result = Box_callEngineNodeLiteral(TOY_AS_OPAQUE(nodeLiteral), interpreter, fnNameIdentifier, &extraArgs);

	Toy_pushLiteralArray(&interpreter->stack, result);

	//cleanup
	Toy_freeLiteralArray(&extraArgs);
	Toy_freeLiteral(fnNameIdentifier);
	Toy_freeLiteral(nodeLiteral);
	Toy_freeLiteral(fnName);
	Toy_freeLiteral(result);

	return 1;
}

//call the hook
typedef struct Natives {
	char* name;
	Toy_NativeFn fn;
} Natives;

int Box_hookNode(Toy_Interpreter* interpreter, Toy_Literal identifier, Toy_Literal alias) {
	//build the natives list
	Natives natives[] = {
		{"loadNode", nativeLoadNode},
		{"initNode", nativeInitNode},
		{"freeChildNode", nativeFreeChildNode},
		{"pushNode", nativePushNode},
		{"getNodeChild", nativeGetNodeChild},
		{"getNodeParent", nativeGetNodeParent},
		{"loadTexture", nativeLoadTexture},
		{"freeTexture", nativeFreeTexture},
		{"setRect", nativeSetRect},
		{"drawNode", nativeDrawNode},
		{"callNode", nativeCallNode},
		{NULL, NULL},
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
