#include "engine_node.h"

#include "lexer.h"
#include "parser.h"
#include "compiler.h"
#include "interpreter.h"
#include "repl_tools.h"
#include "console_colors.h"

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
		EngineNode node;

		initEngineNode(&node, &interpreter, tb, size);

		callEngineNode(&node, &interpreter, "onInit");
		callEngineNode(&node, &interpreter, "onStep");
		callEngineNode(&node, &interpreter, "onFree");

		freeEngineNode(&node);

		//free
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
		EngineNode node;

		initEngineNode(&node, &interpreter, tb, size);
		resetInterpreter(&interpreter);

		for (int i = 0; i < 10; i++) {
			char* source = readFile("./scripts/child_engine_node.toy", &size);
			unsigned char* tb = compileString(source, &size);

			EngineNode child;
			initEngineNode(&child, &interpreter, tb, size);
			resetInterpreter(&interpreter);

			pushEngineNode(&node, &child);

			free((void*)source);
		}

		//test the calls
		callEngineNode(&node, &interpreter, "onInit");

		for (int i = 0; i < 10; i++) {
			callEngineNode(&node, &interpreter, "onStep");
		}

		callEngineNode(&node, &interpreter, "onFree");

		//free
		freeEngineNode(&node);
		free((void*)source);
		freeInterpreter(&interpreter);
	}

	printf(NOTICE "All good\n" RESET);
	return 0;
}
