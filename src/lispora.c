#include <stdio.h>
#include <stdlib.h>

#include "../lib/mpc/mpc.h"

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

// Types of lval
enum { LVAL_NUM, LVAL_ERR };

// Types of errors
enum { ERR_DIV_ZERO, ERR_BAD_OP, ERR_BAD_NUM };

typedef struct {
	int type; // Number or Error
	long num;
	int err;
} lval;

lval lval_num(long num) {
	lval v;
	v.type = LVAL_NUM;
	v.num = num;
	return v;
}

lval lval_err(int err) {
	lval v;
	v.type = LVAL_ERR;
	v.err = err;
	return v;
}

void lval_print(lval v) {
	switch (v.type) {
		case LVAL_NUM:
			printf("%li", v.num);
			break;
		case LVAL_ERR:
			switch (v.err) {
				case ERR_DIV_ZERO:
					printf("Error: Division by zero.");
					break;
				case ERR_BAD_OP:
					printf("Error: Invalid operator.");
					break;
				case ERR_BAD_NUM:
					printf("Error: Invalid number.");
					break;
			}
			break;
	}
}

void lval_println(lval v) {
	lval_print(v);
	putchar('\n');
}

lval eval_op(lval x, char* operator, lval y) {
	if (x.type == LVAL_ERR) { return x; }
	if (y.type == LVAL_ERR) { return y; }

	if (!strcmp(operator, "+")) {
		return lval_num(x.num + y.num);
	} else if (!strcmp(operator, "-")) {
		return lval_num(x.num - y.num);
	} else if (!strcmp(operator, "*")) {
		return lval_num(x.num * y.num);
	} else if (!strcmp(operator, "/")) {
		if (y.num == 0) {
			return lval_err(ERR_DIV_ZERO);
		} else {
			return lval_num(x.num / y.num);
		}
	} else {
		return lval_err(ERR_BAD_OP);
	}
}

lval eval(mpc_ast_t* ast) {
	// If it is a number, convert it to long and return the result
	if (strstr(ast->tag, "number")) {
		errno = 0;
		long x = strtol(ast->contents, NULL, 10);
		if (errno != ERANGE) {
			return lval_num(x);
		} else {
			return lval_err(ERR_BAD_NUM);
		}
	}

	// The first child (witd index 0) is always the character '('
	char* operator = ast->children[1]->contents;

	lval x = eval(ast->children[2]);

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

	puts("Lispora version 0.0.0.0.0.0.1");
	puts("Press Ctrl-C for exit.");

	while (1) {
		char* input = readline("lispora> ");

		add_history(input);

		mpc_result_t res;

		// Parse input from the stdin as a Program and copy the result to res.
		// If it is successful, return 1, otherwise 0.
		int success = mpc_parse("<stdin>", input, Program, &res);

		if (success) {
			lval result = eval(res.output);
			lval_println(result);
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
