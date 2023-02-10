#include "box_engine_node.h"

#include "toy_lexer.h"
#include "toy_parser.h"
#include "toy_compiler.h"
#include "toy_interpreter.h"
#include "toy_console_colors.h"
#include "toy_memory.h"

#include "repl_tools.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//suppress the print keyword
static void noPrintFn(const char* output) {
	//NO OP
}

int main() {
	{
		//setup interpreter
		Toy_Interpreter interpreter;
		Toy_initInterpreter(&interpreter);
		Toy_setInterpreterPrint(&interpreter, noPrintFn);

		size_t size = 0;

		const char* source = Toy_readFile("./scripts/parent_engine_node.toy", &size);
		const unsigned char* tb = Toy_compileString(source, &size);

		//create and test the engine node
		Box_EngineNode* node = TOY_ALLOCATE(Box_EngineNode, 1);

		Box_initEngineNode(node, &interpreter, tb, size);

		Toy_Literal nodeLiteral = TOY_TO_OPAQUE_LITERAL(node, OPAQUE_TAG_ENGINE_NODE);

		//argument list to pass in the node
		Toy_LiteralArray arguments;
		Toy_initLiteralArray(&arguments);

		//call each function
		Box_callRecursiveEngineNode(node, &interpreter, "onInit", &arguments);
		Box_callRecursiveEngineNode(node, &interpreter, "onStep", &arguments);
		Box_callRecursiveEngineNode(node, &interpreter, "onQuit", &arguments);

		//cleanup
		Toy_freeLiteralArray(&arguments);
		Box_freeEngineNode(node);
		free((void*)source);

		Toy_freeInterpreter(&interpreter);
	}

	{
		//setup interpreter
		Toy_Interpreter interpreter;
		Toy_initInterpreter(&interpreter);
		Toy_setInterpreterPrint(&interpreter, noPrintFn);

		size_t size = 0;

		const char* source = Toy_readFile("./scripts/parent_engine_node.toy", &size);
		const unsigned char* tb = Toy_compileString(source, &size);

		//create and test the engine node
		Box_EngineNode* node = TOY_ALLOCATE(Box_EngineNode, 1);

		Box_initEngineNode(node, &interpreter, tb, size);
		Toy_resetInterpreter(&interpreter);

		for (int i = 0; i < 10; i++) {
			const char* source = Toy_readFile("./scripts/child_engine_node.toy", &size);
			const unsigned char* tb = Toy_compileString(source, &size);

			Box_EngineNode* child = TOY_ALLOCATE(Box_EngineNode, 1);
			Box_initEngineNode(child, &interpreter, tb, size);
			Toy_resetInterpreter(&interpreter);

			Box_pushEngineNode(node, child);

			free((void*)source);
		}

		//argument list to pass in each time
		Toy_LiteralArray arguments;
		Toy_initLiteralArray(&arguments);

		//test the calls
		Box_callRecursiveEngineNode(node, &interpreter, "onInit", &arguments);

		for (int i = 0; i < 10; i++) {
			Box_callRecursiveEngineNode(node, &interpreter, "onStep", &arguments);
		}

		Box_callRecursiveEngineNode(node, &interpreter, "onFree", &arguments);

		//cleanup
		Toy_freeLiteralArray(&arguments);
		Box_freeEngineNode(node); //frees all children
		free((void*)source);

		Toy_freeInterpreter(&interpreter);
	}

	printf(TOY_CC_NOTICE "All good\n" TOY_CC_RESET);
	return 0;
}
