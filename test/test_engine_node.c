#include "engine_node.h"

#include "lexer.h"
#include "parser.h"
#include "compiler.h"
#include "interpreter.h"
#include "console_colors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//suppress the print keyword
static void noPrintFn(const char* output) {
	//NO OP
}

//compilation functions
char* readFile(char* path, size_t* fileSize) {
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

unsigned char* compileString(char* source, size_t* size) {
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
