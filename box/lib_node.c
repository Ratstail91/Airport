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
	const unsigned char* source = Toy_readFile(Toy_toCString(TOY_AS_STRING(filePathLiteral)), &size);
	const unsigned char* tb = Toy_compileString((const char*)source, &size);
	free((void*)source);

	Box_EngineNode* node = TOY_ALLOCATE(Box_EngineNode, 1);

	//BUGFIX: make an -interpreter
	Toy_Interpreter inner;

	//init the inner interpreter manually
	Toy_initLiteralArray(&inner.literalCache);
	Toy_initLiteralArray(&inner.stack);
	inner.hooks = interpreter->hooks;
	inner.scope = Toy_pushScope(interpreter->scope);
	inner.bytecode = tb;
	inner.length = size;
	inner.count = 0;
	inner.codeStart = -1;
	inner.depth = interpreter->depth + 1;
	inner.panic = false;
	Toy_setInterpreterPrint(&inner, interpreter->printOutput);
	Toy_setInterpreterAssert(&inner, interpreter->assertOutput);
	Toy_setInterpreterError(&inner, interpreter->errorOutput);

	Box_initEngineNode(node, &inner, tb, size);

	//immediately call onLoad() after running the script - for loading other nodes
	Box_callEngineNode(node, &inner, "onLoad", NULL);

	// return the node
	Toy_Literal nodeLiteral = TOY_TO_OPAQUE_LITERAL(node, node->tag);
	Toy_pushLiteralArray(&interpreter->stack, nodeLiteral);

	//cleanup (NOT the scope - that needs to hang around)
	node->scope = inner.scope;

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

static int nativeGetChildNode(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	//checks
	if (arguments->count != 2) {
		interpreter->errorOutput("Incorrect number of arguments passed to getChildNode\n");
		return -1;
	}

	Toy_Literal index = Toy_popLiteralArray(arguments);
	Toy_Literal parent = Toy_popLiteralArray(arguments);

	Toy_Literal parentIdn = parent;
	if (TOY_IS_IDENTIFIER(parent) && Toy_parseIdentifierToValue(interpreter, &parent)) {
		Toy_freeLiteral(parentIdn);
	}

	Toy_Literal indexIdn = index;
	if (TOY_IS_IDENTIFIER(index) && Toy_parseIdentifierToValue(interpreter, &index)) {
		Toy_freeLiteral(indexIdn);
	}

	if (!TOY_IS_OPAQUE(parent) || !TOY_IS_INTEGER(index)) {
		interpreter->errorOutput("Incorrect argument type passed to getChildNode\n");
		Toy_freeLiteral(parent);
		Toy_freeLiteral(index);
		return -1;
	}

	//push the node
	Box_EngineNode* parentNode = TOY_AS_OPAQUE(parent);
	int intIndex = TOY_AS_INTEGER(index);

	if (intIndex < 0 || intIndex >= parentNode->count) {
		interpreter->errorOutput("index out of bounds in getChildNode\n");
		Toy_freeLiteral(parent);
		Toy_freeLiteral(index);
		return -1;
	}

	Box_EngineNode* childNode = Box_getChildEngineNode(parentNode, intIndex);
	Toy_Literal child;

	if (childNode == NULL) {
		child = TOY_TO_NULL_LITERAL;
	}
	else {
		child = TOY_TO_OPAQUE_LITERAL(childNode, childNode->tag);
	}

	Toy_pushLiteralArray(&interpreter->stack, child);

	//cleanup
	Toy_freeLiteral(parent);
	Toy_freeLiteral(child);
	Toy_freeLiteral(index);

	return 1;
}

static int nativeFreeChildNode(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	if (arguments->count != 2) {
		interpreter->errorOutput("Incorrect number of arguments passed to freeChildNode\n");
		return -1;
	}

	Toy_Literal indexLiteral = Toy_popLiteralArray(arguments);
	Toy_Literal nodeLiteral = Toy_popLiteralArray(arguments);

	Toy_Literal nodeLiteralIdn = nodeLiteral; //annoying
	if (TOY_IS_IDENTIFIER(nodeLiteral) && Toy_parseIdentifierToValue(interpreter, &nodeLiteral)) {
		Toy_freeLiteral(nodeLiteralIdn);
	}

	//check argument types
	if (!TOY_IS_OPAQUE(nodeLiteral) || !TOY_IS_INTEGER(indexLiteral)) {
		interpreter->errorOutput("Incorrect argument type passed to freeChildNode\n");
		Toy_freeLiteral(nodeLiteral);
		return -1;
	}

	Box_EngineNode* node = TOY_AS_OPAQUE(nodeLiteral);
	int idx = TOY_AS_INTEGER(indexLiteral);

	//check bounds
	if (idx < 0 || idx >= node->count) {
		interpreter->errorOutput("Node index out of bounds in freeChildNode\n");
		Toy_freeLiteral(nodeLiteral);
		Toy_freeLiteral(indexLiteral);
		return -1;
	}

	//TODO: differentiate between onFree() and freeing memory
	Box_callRecursiveEngineNode(node, interpreter, "onFree", NULL);
	Box_freeChildEngineNode(node, idx);

	//cleanup
	Toy_freeLiteral(nodeLiteral);
	Toy_freeLiteral(indexLiteral);
	return 0;
}

static int nativeGetParentNode(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	//checks
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to getParentNode\n");
		return -1;
	}

	Toy_Literal nodeLiteral = Toy_popLiteralArray(arguments);

	Toy_Literal nodeIdn = nodeLiteral;
	if (TOY_IS_IDENTIFIER(nodeLiteral) && Toy_parseIdentifierToValue(interpreter, &nodeLiteral)) {
		Toy_freeLiteral(nodeIdn);
	}

	if (!TOY_IS_OPAQUE(nodeLiteral)) {
		interpreter->errorOutput("Incorrect argument type passed to getParentNode\n");
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

static int nativeGetChildNodeCount(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	//checks
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to getChildNodeCount\n");
		return -1;
	}

	Toy_Literal nodeLiteral = Toy_popLiteralArray(arguments);

	Toy_Literal nodeLiteralIdn = nodeLiteral;
	if (TOY_IS_IDENTIFIER(nodeLiteral) && Toy_parseIdentifierToValue(interpreter, &nodeLiteral)) {
		Toy_freeLiteral(nodeLiteralIdn);
	}

	if (!TOY_IS_OPAQUE(nodeLiteral)) {
		interpreter->errorOutput("Incorrect argument type passed to getChildNodeCount\n");
		Toy_freeLiteral(nodeLiteral);
		return -1;
	}

	//get the count
	Box_EngineNode* node = TOY_AS_OPAQUE(nodeLiteral);
	int childCount = Box_getChildCountEngineNode(node);
	Toy_Literal childCountLiteral = TOY_TO_INTEGER_LITERAL(childCount);

	Toy_pushLiteralArray(&interpreter->stack, childCountLiteral);

	//no return value
	Toy_freeLiteral(nodeLiteral);
	Toy_freeLiteral(childCountLiteral);

	return 0;
}

static int nativeLoadNodeTexture(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	if (arguments->count != 2) {
		interpreter->errorOutput("Incorrect number of arguments passed to loadNodeTexture\n");
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
		interpreter->errorOutput("Incorrect argument type passed to loadNodeTexture\n");
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

	//actually load TODO: number the opaques, and check the tag
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

static int nativeFreeNodeTexture(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to freeNodeTexture\n");
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
		interpreter->errorOutput("Incorrect argument type passed to freeNodeTexture\n");
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

static int nativeSetNodeRect(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	if (arguments->count != 5) {
		interpreter->errorOutput("Incorrect number of arguments passed to setNodeRect\n");
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
		interpreter->errorOutput("Incorrect argument type passed to setNodeRect\n");
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

static int nativeGetNodeRectX(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to getNodeRectX\n");
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
		interpreter->errorOutput("Incorrect argument type passed to getNodeRectX\n");
		Toy_freeLiteral(nodeLiteral);
		return -1;
	}

	//actually get
	Box_EngineNode* node = (Box_EngineNode*)TOY_AS_OPAQUE(nodeLiteral);
	Toy_Literal resultLiteral = TOY_TO_INTEGER_LITERAL(node->rect.x);
	Toy_pushLiteralArray(&interpreter->stack, resultLiteral);

	//cleanup
	Toy_freeLiteral(nodeLiteral);
	Toy_freeLiteral(resultLiteral);

	return 1;
}

static int nativeGetNodeRectY(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to getNodeRectY\n");
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
		interpreter->errorOutput("Incorrect argument type passed to getNodeRectY\n");
		Toy_freeLiteral(nodeLiteral);
		return -1;
	}

	//actually get
	Box_EngineNode* node = (Box_EngineNode*)TOY_AS_OPAQUE(nodeLiteral);
	Toy_Literal resultLiteral = TOY_TO_INTEGER_LITERAL(node->rect.y);
	Toy_pushLiteralArray(&interpreter->stack, resultLiteral);

	//cleanup
	Toy_freeLiteral(nodeLiteral);
	Toy_freeLiteral(resultLiteral);

	return 1;
}

static int nativeGetNodeRectW(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to getNodeRectW\n");
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
		interpreter->errorOutput("Incorrect argument type passed to getNodeRectW\n");
		Toy_freeLiteral(nodeLiteral);
		return -1;
	}

	//actually get
	Box_EngineNode* node = (Box_EngineNode*)TOY_AS_OPAQUE(nodeLiteral);
	Toy_Literal resultLiteral = TOY_TO_INTEGER_LITERAL(node->rect.w);
	Toy_pushLiteralArray(&interpreter->stack, resultLiteral);

	//cleanup
	Toy_freeLiteral(nodeLiteral);
	Toy_freeLiteral(resultLiteral);

	return 1;
}

static int nativeGetNodeRectH(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to getNodeRectH\n");
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
		interpreter->errorOutput("Incorrect argument type passed to getNodeRectH\n");
		Toy_freeLiteral(nodeLiteral);
		return -1;
	}

	//actually get
	Box_EngineNode* node = (Box_EngineNode*)TOY_AS_OPAQUE(nodeLiteral);
	Toy_Literal resultLiteral = TOY_TO_INTEGER_LITERAL(node->rect.h);
	Toy_pushLiteralArray(&interpreter->stack, resultLiteral);

	//cleanup
	Toy_freeLiteral(nodeLiteral);
	Toy_freeLiteral(resultLiteral);

	return 1;
}

static int nativeSetNodeFrames(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	if (arguments->count != 2) {
		interpreter->errorOutput("Incorrect number of arguments passed to setNodeFrames\n");
		return -1;
	}

	//extract the arguments
	Toy_Literal framesLiteral = Toy_popLiteralArray(arguments);
	Toy_Literal nodeLiteral = Toy_popLiteralArray(arguments);

	Toy_Literal nodeIdn = nodeLiteral;
	if (TOY_IS_IDENTIFIER(nodeLiteral) && Toy_parseIdentifierToValue(interpreter, &nodeLiteral)) {
		Toy_freeLiteral(nodeIdn);
	}

	Toy_Literal frameLiteralIdn = framesLiteral;
	if (TOY_IS_IDENTIFIER(framesLiteral) && Toy_parseIdentifierToValue(interpreter, &framesLiteral)) {
		Toy_freeLiteral(frameLiteralIdn);
	}

	//check argument types
	if (!TOY_IS_OPAQUE(nodeLiteral) || !TOY_IS_INTEGER(framesLiteral)) {
		interpreter->errorOutput("Incorrect argument type passed to setNodeFrames\n");
		Toy_freeLiteral(nodeLiteral);
		Toy_freeLiteral(framesLiteral);
		return -1;
	}

	//actually set
	Box_EngineNode* node = (Box_EngineNode*)TOY_AS_OPAQUE(nodeLiteral);

	Box_setFramesEngineNode(node, TOY_AS_INTEGER(framesLiteral));

	//cleanup
	Toy_freeLiteral(nodeLiteral);
	Toy_freeLiteral(framesLiteral);

	return 0;
}

static int nativeGetNodeFrames(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to getNodeFrames\n");
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
		interpreter->errorOutput("Incorrect argument type passed to getNodeFrames\n");
		Toy_freeLiteral(nodeLiteral);
		return -1;
	}

	//actually get
	Box_EngineNode* node = (Box_EngineNode*)TOY_AS_OPAQUE(nodeLiteral);
	Toy_Literal framesLiteral = TOY_TO_INTEGER_LITERAL(node->frames);

	Toy_pushLiteralArray(&interpreter->stack, framesLiteral);


	//cleanup
	Toy_freeLiteral(nodeLiteral);
	Toy_freeLiteral(framesLiteral);

	return 1;
}

static int nativeSetCurrentNodeFrame(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	if (arguments->count != 2) {
		interpreter->errorOutput("Incorrect number of arguments passed to setCurrentNodeFrame\n");
		return -1;
	}

	//extract the arguments
	Toy_Literal currentFrameLiteral = Toy_popLiteralArray(arguments);
	Toy_Literal nodeLiteral = Toy_popLiteralArray(arguments);

	Toy_Literal nodeIdn = nodeLiteral;
	if (TOY_IS_IDENTIFIER(nodeLiteral) && Toy_parseIdentifierToValue(interpreter, &nodeLiteral)) {
		Toy_freeLiteral(nodeIdn);
	}

	Toy_Literal currentFrameLiteralIdn = currentFrameLiteral;
	if (TOY_IS_IDENTIFIER(currentFrameLiteral) && Toy_parseIdentifierToValue(interpreter, &currentFrameLiteral)) {
		Toy_freeLiteral(currentFrameLiteralIdn);
	}

	//check argument types
	if (!TOY_IS_OPAQUE(nodeLiteral) || !TOY_IS_INTEGER(currentFrameLiteral)) {
		interpreter->errorOutput("Incorrect argument type passed to setCurrentNodeFrame\n");
		Toy_freeLiteral(nodeLiteral);
		Toy_freeLiteral(currentFrameLiteral);
		return -1;
	}

	//actually set
	Box_EngineNode* node = (Box_EngineNode*)TOY_AS_OPAQUE(nodeLiteral);

	Box_setCurrentFrameEngineNode(node, TOY_AS_INTEGER(currentFrameLiteral));

	//cleanup
	Toy_freeLiteral(nodeLiteral);
	Toy_freeLiteral(currentFrameLiteral);

	return 0;
}

static int nativeGetCurrentNodeFrame(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to getCurrentNodeFrame\n");
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
		interpreter->errorOutput("Incorrect argument type passed to getCurrentNodeFrame\n");
		Toy_freeLiteral(nodeLiteral);
		return -1;
	}

	//actually get
	Box_EngineNode* node = (Box_EngineNode*)TOY_AS_OPAQUE(nodeLiteral);
	Toy_Literal currentFrameLiteral = TOY_TO_INTEGER_LITERAL(node->currentFrame);

	Toy_pushLiteralArray(&interpreter->stack, currentFrameLiteral);

	//cleanup
	Toy_freeLiteral(nodeLiteral);
	Toy_freeLiteral(currentFrameLiteral);

	return 1;
}

static int nativeIncrementCurrentNodeFrame(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to incrementCurrentNodeFrame\n");
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
		interpreter->errorOutput("Incorrect argument type passed to incrementCurrentNodeFrame\n");
		Toy_freeLiteral(nodeLiteral);
		return -1;
	}

	//actually get
	Box_EngineNode* node = (Box_EngineNode*)TOY_AS_OPAQUE(nodeLiteral);

	Box_incrementCurrentFrame(node);

	//cleanup
	Toy_freeLiteral(nodeLiteral);

	return 0;
}

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

static int nativeSetNodeText(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	if (arguments->count != 8) {
		interpreter->errorOutput("Incorrect number of arguments passed to setNodeText\n");
		return -1;
	}

	//extract the arguments
	Toy_Literal aLiteral = Toy_popLiteralArray(arguments);
	Toy_Literal bLiteral = Toy_popLiteralArray(arguments);
	Toy_Literal gLiteral = Toy_popLiteralArray(arguments);
	Toy_Literal rLiteral = Toy_popLiteralArray(arguments);
	Toy_Literal textLiteral = Toy_popLiteralArray(arguments);
	Toy_Literal sizeLiteral = Toy_popLiteralArray(arguments);
	Toy_Literal fontLiteral = Toy_popLiteralArray(arguments);
	Toy_Literal nodeLiteral = Toy_popLiteralArray(arguments);

	Toy_Literal nodeLiteralIdn = nodeLiteral;
	if (TOY_IS_IDENTIFIER(nodeLiteral) && Toy_parseIdentifierToValue(interpreter, &nodeLiteral)) {
		Toy_freeLiteral(nodeLiteralIdn);
	}

	Toy_Literal fontLiteralIdn = fontLiteral;
	if (TOY_IS_IDENTIFIER(fontLiteral) && Toy_parseIdentifierToValue(interpreter, &fontLiteral)) {
		Toy_freeLiteral(fontLiteralIdn);
	}

	Toy_Literal sizeLiteralIdn = sizeLiteral;
	if (TOY_IS_IDENTIFIER(sizeLiteral) && Toy_parseIdentifierToValue(interpreter, &sizeLiteral)) {
		Toy_freeLiteral(sizeLiteralIdn);
	}

	Toy_Literal textLiteralIdn = textLiteral;
	if (TOY_IS_IDENTIFIER(textLiteral) && Toy_parseIdentifierToValue(interpreter, &textLiteral)) {
		Toy_freeLiteral(textLiteralIdn);
	}

	Toy_Literal rLiteralIdn = rLiteral;
	if (TOY_IS_IDENTIFIER(rLiteral) && Toy_parseIdentifierToValue(interpreter, &rLiteral)) {
		Toy_freeLiteral(rLiteralIdn);
	}

	Toy_Literal gLiteralIdn = gLiteral;
	if (TOY_IS_IDENTIFIER(gLiteral) && Toy_parseIdentifierToValue(interpreter, &gLiteral)) {
		Toy_freeLiteral(gLiteralIdn);
	}

	Toy_Literal bLiteralIdn = bLiteral;
	if (TOY_IS_IDENTIFIER(bLiteral) && Toy_parseIdentifierToValue(interpreter, &bLiteral)) {
		Toy_freeLiteral(bLiteralIdn);
	}

	Toy_Literal aLiteralIdn = aLiteral;
	if (TOY_IS_IDENTIFIER(aLiteral) && Toy_parseIdentifierToValue(interpreter, &aLiteral)) {
		Toy_freeLiteral(aLiteralIdn);
	}

	//check argument types
	if (!TOY_IS_OPAQUE(nodeLiteral) || !TOY_IS_STRING(fontLiteral)  || !TOY_IS_INTEGER(sizeLiteral) || !TOY_IS_STRING(textLiteral)
		|| !TOY_IS_INTEGER(rLiteral) || !TOY_IS_INTEGER(gLiteral) || !TOY_IS_INTEGER(bLiteral) || !TOY_IS_INTEGER(aLiteral)) {
		interpreter->errorOutput("Incorrect argument type passed to setNodeText\n");
		Toy_freeLiteral(nodeLiteral);
		Toy_freeLiteral(fontLiteral);
		Toy_freeLiteral(sizeLiteral);
		Toy_freeLiteral(textLiteral);
		Toy_freeLiteral(rLiteral);
		Toy_freeLiteral(gLiteral);
		Toy_freeLiteral(bLiteral);
		Toy_freeLiteral(aLiteral);
		return -1;
	}

	//bounds checks
	if (TOY_AS_INTEGER(rLiteral) < 0 || TOY_AS_INTEGER(rLiteral) > 255 ||
		TOY_AS_INTEGER(gLiteral) < 0 || TOY_AS_INTEGER(gLiteral) > 255 ||
		TOY_AS_INTEGER(bLiteral) < 0 || TOY_AS_INTEGER(bLiteral) > 255 ||
		TOY_AS_INTEGER(aLiteral) < 0 || TOY_AS_INTEGER(aLiteral) > 255) {
		interpreter->errorOutput("Color out of bounds in to setNodeText\n");
		Toy_freeLiteral(nodeLiteral);
		Toy_freeLiteral(fontLiteral);
		Toy_freeLiteral(sizeLiteral);
		Toy_freeLiteral(textLiteral);
		Toy_freeLiteral(rLiteral);
		Toy_freeLiteral(gLiteral);
		Toy_freeLiteral(bLiteral);
		Toy_freeLiteral(aLiteral);
		return -1;
	}

	//get the font
	Toy_Literal fileLiteral = Toy_getFilePathLiteral(interpreter, &fontLiteral);

	TTF_Font* font = TTF_OpenFont( Toy_toCString(TOY_AS_STRING(fileLiteral)), TOY_AS_INTEGER(sizeLiteral) );

	if (!font) {
		interpreter->errorOutput("Failed to open a font file: ");
		interpreter->errorOutput(SDL_GetError());
		interpreter->errorOutput("\n");

		Toy_freeLiteral(fileLiteral);
		Toy_freeLiteral(nodeLiteral);
		Toy_freeLiteral(fontLiteral);
		Toy_freeLiteral(sizeLiteral);
		Toy_freeLiteral(textLiteral);
		Toy_freeLiteral(rLiteral);
		Toy_freeLiteral(gLiteral);
		Toy_freeLiteral(bLiteral);
		Toy_freeLiteral(aLiteral);
		return -1;
	}

	//make the color
	SDL_Color color = (SDL_Color){ .r = TOY_AS_INTEGER(rLiteral), .g = TOY_AS_INTEGER(gLiteral), .b = TOY_AS_INTEGER(bLiteral), .a = TOY_AS_INTEGER(aLiteral) };

	//actually set
	Box_EngineNode* node = (Box_EngineNode*)TOY_AS_OPAQUE(nodeLiteral);
	Box_setTextEngineNode(node, font, Toy_toCString(TOY_AS_STRING(textLiteral)), color);

	//cleanup
	TTF_CloseFont(font);

	Toy_freeLiteral(fileLiteral);
	Toy_freeLiteral(nodeLiteral);
	Toy_freeLiteral(fontLiteral);
	Toy_freeLiteral(sizeLiteral);
	Toy_freeLiteral(textLiteral);
	Toy_freeLiteral(rLiteral);
	Toy_freeLiteral(gLiteral);
	Toy_freeLiteral(bLiteral);
	Toy_freeLiteral(aLiteral);

	return 0;
}

static int nativeCallNodeFn(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	//checks
	if (arguments->count < 2) {
		interpreter->errorOutput("Too few arguments passed to callNodeFn\n");
		return -1;
	}

	Toy_LiteralArray extraArgsBackwards;
	Toy_initLiteralArray(&extraArgsBackwards);

	//extract the extra arg values
	while (arguments->count > 2) {
		Toy_Literal tmp = Toy_popLiteralArray(arguments);

		Toy_Literal idn = tmp; //there's almost certainly a better way of doing all of this stuff
		if (TOY_IS_IDENTIFIER(tmp) && Toy_parseIdentifierToValue(interpreter, &tmp)) {
			Toy_freeLiteral(idn);
		}

		Toy_pushLiteralArray(&extraArgsBackwards, tmp);
		Toy_freeLiteral(tmp);
	}

	//reverse the extra args
	Toy_LiteralArray extraArgs;
	Toy_initLiteralArray(&extraArgs);

	while (extraArgsBackwards.count > 0) {
		Toy_Literal tmp = Toy_popLiteralArray(&extraArgsBackwards);
		Toy_pushLiteralArray(&extraArgs, tmp);
		Toy_freeLiteral(tmp);
	}

	Toy_freeLiteralArray(&extraArgsBackwards);

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
		interpreter->errorOutput("Incorrect argument type passed to callNodeFn\n");
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
		{"pushNode", nativePushNode},
		{"getChildNode", nativeGetChildNode},
		{"freeChildNode", nativeFreeChildNode},
		{"getParentNode", nativeGetParentNode},
		{"getChildNodeCount", nativeGetChildNodeCount},
		{"loadNodeTexture", nativeLoadNodeTexture},
		{"freeNodeTexture", nativeFreeNodeTexture},
		{"setNodeRect", nativeSetNodeRect},
		{"getNodeRectX", nativeGetNodeRectX},
		{"getNodeRectY", nativeGetNodeRectY},
		{"getNodeRectW", nativeGetNodeRectW},
		{"getNodeRectH", nativeGetNodeRectH},
		{"setNodeFrames", nativeSetNodeFrames},
		{"getNodeFrames", nativeGetNodeFrames},
		{"setCurrentNodeFrame", nativeSetCurrentNodeFrame},
		{"getCurrentNodeFrame", nativeGetCurrentNodeFrame},
		{"incrementCurrentNodeFrame", nativeIncrementCurrentNodeFrame},
		{"drawNode", nativeDrawNode},
		{"setNodeText", nativeSetNodeText},
		{"callNodeFn", nativeCallNodeFn},

		//TODO: get rect, get node var, create empty node, set node color (tinting)
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
