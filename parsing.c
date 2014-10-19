#include <stdio.h>
#include <stdlib.h>

#include "mpc.h"

#ifdef _WIN32

#include <string.h>

static char buffer[2048];

char* readline(char* prompt) {
	fputs(prompt, stdout);
	fgets(buffer, 2048, stdin);
	char* cpy = malloc(strlen(buffer) + 1);
	strcpy(cpy, buffer);
	cpy[strlen(cpy) - 1] = '\0';
	return cpy;
}

void add_history(char* unused) {};

#else

#include <editline/readline.h>

#endif

long eval_op(long x, char* operator, long y) {
	if (!strcmp(operator, "+")) {
		return x + y;
	} else if (!strcmp(operator, "-")) {
		return x - y;
	} else if (!strcmp(operator, "*")) {
		return x * y;
	} else if (!strcmp(operator, "/")) {
		return x / y;
	}

	return 0;
}

long eval(mpc_ast_t* ast) {
	// If it is a number, convert it to long and return the result
	if (strstr(ast->tag, "number")) {
		return atoi(ast->contents);
	}

	// The first child (witd index 0) is always the character '('
	char* operator = ast->children[1]->contents;

	long x = eval(ast->children[2]);

	// Untill you reach the end of the expression...
	int i = 3;
	while(strstr(ast->children[i]->tag, "expression")) {
		x = eval_op(x, operator, eval(ast->children[i]));
		i++;
	}

	return x;
}

int main(int argc, char *argv[]) {

	mpc_parser_t* Number     = mpc_new("number");
	mpc_parser_t* Operator   = mpc_new("operator");
	mpc_parser_t* Expression = mpc_new("expression");
	mpc_parser_t* Program    = mpc_new("program");

	mpca_lang(MPCA_LANG_DEFAULT,
		"                                                              \
			number     : /-?[0-9]+/ ;                      \
			operator   : '+' | '-' | '*' | '/' ;                       \
			expression : <number> | '(' <operator> <expression>+ ')' ; \
			program    : /^/ <operator> <expression>+ /$/ ;            \
		",
		Number, Operator, Expression, Program);

	puts("Lispora version beta 0.0.0.0.0.0.1");
	puts("Press Ctrl-C for exit.");

	while (1) {
		char* input = readline("lispora> ");

		add_history(input);

		mpc_result_t res;

		// Parse input from the stdin as a Program and copy the result to res.
		// If it is successful, return 1, otherwise 0.
		int success = mpc_parse("<stdin>", input, Program, &res);

		if (success) {
			long result = eval(res.output);
			printf("%li\n", result);
			mpc_ast_delete(res.output);
		} else {
			mpc_err_print(res.error);
			mpc_err_delete(res.error);
		}

		free(input);
	}

	mpc_cleanup(4, Number, Operator, Expression, Program);

	return 0;
}
