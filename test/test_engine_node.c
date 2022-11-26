#include "engine_node.h"

#include "lexer.h"
#include "parser.h"
#include "compiler.h"
#include "interpreter.h"
#include "repl_tools.h"
#include "console_colors.h"
#include "memory.h"

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
		Interpreter interpreter;
		initInterpreter(&interpreter);
		setInterpreterPrint(&interpreter, noPrintFn);

		size_t size = 0;

		char* source = readFile("./scripts/parent_engine_node.toy", &size);
		unsigned char* tb = compileString(source, &size);

		//create and test the engine node
		EngineNode* node = ALLOCATE(EngineNode, 1);

		initEngineNode(node, &interpreter, tb, size);

		Literal nodeLiteral = TO_OPAQUE_LITERAL(node, 0);

		//argument list to pass in the node
		LiteralArray arguments;
		initLiteralArray(&arguments);

		//call each function
		callRecursiveEngineNode(node, &interpreter, "onInit", &arguments);
		callRecursiveEngineNode(node, &interpreter, "onStep", &arguments);
		callRecursiveEngineNode(node, &interpreter, "onQuit", &arguments);

		//cleanup
		freeLiteralArray(&arguments);
		freeEngineNode(node);
		free((void*)source);
		freeInterpreter(&interpreter);
	}

	{
		//setup interpreter
		Interpreter interpreter;
		initInterpreter(&interpreter);
		setInterpreterPrint(&interpreter, noPrintFn);

		size_t size = 0;

		char* source = readFile("./scripts/parent_engine_node.toy", &size);
		unsigned char* tb = compileString(source, &size);

		//create and test the engine node
		EngineNode* node = ALLOCATE(EngineNode, 1);

		initEngineNode(node, &interpreter, tb, size);
		resetInterpreter(&interpreter);

		for (int i = 0; i < 10; i++) {
			char* source = readFile("./scripts/child_engine_node.toy", &size);
			unsigned char* tb = compileString(source, &size);

			EngineNode* child = ALLOCATE(EngineNode, 1);
			initEngineNode(child, &interpreter, tb, size);
			resetInterpreter(&interpreter);

			pushEngineNode(node, child);

			free((void*)source);
		}

		//argument list to pass in each time
		LiteralArray arguments;
		initLiteralArray(&arguments);

		//test the calls
		callRecursiveEngineNode(node, &interpreter, "onInit", &arguments);

		for (int i = 0; i < 10; i++) {
			callRecursiveEngineNode(node, &interpreter, "onStep", &arguments);
		}

		callRecursiveEngineNode(node, &interpreter, "onFree", &arguments);

		//cleanup
		freeLiteralArray(&arguments);
		freeEngineNode(node); //frees all children
		free((void*)source);
		freeInterpreter(&interpreter);
	}

	printf(NOTICE "All good\n" RESET);
	return 0;
}
