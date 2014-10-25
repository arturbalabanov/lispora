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
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR };

typedef struct lval {
	int type;

	long num;

	char* err;
	char* sym;

	int count;
	struct lval** cell;
} lval;

void lval_print(lval* v);

lval* buildin_op(lval* args, char* op);

lval* lval_eval(lval* v);

lval* lval_num(long num) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->num = num;
	return v;
}

lval* lval_err(char* err) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_ERR;
	v->err = malloc(strlen(err) + 1);
	strcpy(v->err, err);
	return v;
}

lval* lval_sym(char* sym) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SYM;
	v->sym = malloc(strlen(sym) + 1);
	strcpy(v->sym, sym);
	return v;
}

lval* lval_sexpr(void) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}

lval* lval_qexpr(void) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_QEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}

void lval_del(lval* v) {
	switch (v->type) {
		case LVAL_NUM:
			break;
		case LVAL_SYM:
			free(v->sym);
			break;
		case LVAL_ERR:
			free(v->err);
			break;
		case LVAL_QEXPR:
		case LVAL_SEXPR:
			for (int i = 0; i < v->count; ++i) {
				lval_del(v->cell[i]);
			}

			free(v->cell);
			break;
	}

	free(v);
}

lval* lval_add(lval* v, lval* x) {
	v->count++;
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	v->cell[v->count-1] = x;
	return v;
}

lval* lval_read_num(mpc_ast_t* tree) {
	errno = 0;
	long x = strtol(tree->contents, NULL, 10);
	if (errno != ERANGE) {
		return lval_num(x);
	} else {
		return lval_err("Invalid number");
	}
}

lval* lval_read(mpc_ast_t* tree) {
	if (strstr(tree->tag, "number")) {
		return lval_read_num(tree);
	}

	if (strstr(tree->tag, "symbol")) {
		return lval_sym(tree->contents);
	}

	lval* x = NULL;
	if ((!strcmp(tree->tag, ">")) || strstr(tree->tag, "sexpr")) {
		x = lval_sexpr();
	}

	if (strstr(tree->tag, "qexpr")) {
		x = lval_qexpr();
	}

	int i;
	for (i = 0; i < tree->children_num; ++i) {
		if (!strcmp(tree->children[i]->contents, "(") ||
				!strcmp(tree->children[i]->contents, ")") ||
				!strcmp(tree->children[i]->contents, "{") ||
				!strcmp(tree->children[i]->contents, "}") ||
				!strcmp(tree->children[i]->tag, "regex")) {
			continue;
		}
		x = lval_add(x, lval_read(tree->children[i]));
	}

	return x;
}

void lval_expr_print(lval* v, char open, char close) {
	putchar(open);

	int i;
	for (i = 0; i < v->count; ++i) {
		lval_print(v->cell[i]);
		if (i != v->count-1) {
			putchar(' ');
		}
	}
	putchar(close);
}

void lval_print(lval* v) {
	switch (v->type) {
		case LVAL_NUM:
			printf("%li", v->num);
			break;
		case LVAL_ERR:
			printf("Error: %s", v->err);
			break;
		case LVAL_SYM:
			printf("%s", v->sym);
			break;
		case LVAL_SEXPR:
			lval_expr_print(v, '(', ')');
			break;
		case LVAL_QEXPR:
			lval_expr_print(v, '{', '}');
			break;
	}
}

void lval_println(lval* v) {
	lval_print(v);
	putchar('\n');
}

lval* lval_pop(lval* v, int i) {
	lval* item = v->cell[i];
	memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count - i - 1));
	v->count--;
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);

	return item;
}

lval* lval_take(lval* v, int i) {
	lval* item = lval_pop(v, i);
	lval_del(v);
	return item;
}

lval* lval_eval_sexpr(lval* v) {
	for (int i = 0; i < v->count; ++i) {
		v->cell[i] = lval_eval(v->cell[i]);
	}

	for (int i = 0; i < v->count; ++i) {
		if (v->cell[i]->type == LVAL_ERR) {
			return lval_take(v, i);
		}
	}

	if (v->count == 0) {
		return v;
	}

	if (v->count == 1) {
		return lval_take(v, 0);
	}

	lval* first = lval_pop(v, 0);
	if (first->type != LVAL_SYM) {
		lval_del(first);
		lval_del(v);
		return lval_err("S-expression does not start with symbol.");
	}

	lval* result = buildin_op(v, first->sym);
	lval_del(first);
	return result;
}

lval* buildin_op(lval* args, char* op) {
	for (int i = 0; i < args->count; ++i) {
		if (args->cell[i]->type != LVAL_NUM) {
			lval_del(args);
			return lval_err("Cannot operate on non-numbers.");
		}
	}

	lval* first = lval_pop(args, 0);

	if (!strcmp(op, "-") && args->count == 0) {
		first->num = -first->num;
	}

	while (args->count > 0) {
		lval* next = lval_pop(args, 0);

		if (!strcmp(op, "+")) {
			first->num += next->num;
		} else if (!strcmp(op, "-")) {
			first->num -= next->num;
		} else if (!strcmp(op, "*")) {
			first->num *= next->num;
		} else if (!strcmp(op, "/")) {
			if (next->num == 0) {
				lval_del(first);
				lval_del(next);
				first = lval_err("Division by zero.");
				break;
			}
			first->num /= next->num;
		}

		lval_del(next);
	}

	lval_del(args);
	return first;
}

lval* lval_eval(lval* v) {
	if (v->type == LVAL_SEXPR) {
		return lval_eval_sexpr(v);
	}
	return v;
}

int main(int argc, char *argv[]) {

	mpc_parser_t* Number     = mpc_new("number");
	mpc_parser_t* Symbol     = mpc_new("symbol");
	mpc_parser_t* Sexpr      = mpc_new("sexpr");
	mpc_parser_t* Expr       = mpc_new("expr");
	mpc_parser_t* Qexpr      = mpc_new("qexpr");
	mpc_parser_t* Program    = mpc_new("program");

	mpca_lang(MPCA_LANG_DEFAULT,
		"                                                             \
			number  : /-?[0-9]+/ ;                                    \
			symbol  : '+' | '-' | '*' | '/' ;                         \
			sexpr   : '(' <expr>* ')' ;                               \
			qexpr   : '{' <expr>* '}' ;                               \
			expr    : <number> | <symbol> | <sexpr> | <qexpr> ;       \
			program : /^/ <expr>* /$/ ;                               \
		",
		Number, Symbol, Sexpr, Qexpr, Expr, Program);

	puts("Lispora version 0.0.0.1");
	puts("Press Ctrl-C for exit.");

	while (1) {
		char* input = readline("lispora> ");

		add_history(input);

		mpc_result_t res;

		// Parse input from the stdin as a Program and copy the result to res.
		// If it is successful, return 1, otherwise 0.
		int success = mpc_parse("<stdin>", input, Program, &res);

		if (success) {
			lval* x = lval_eval(lval_read(res.output));
			lval_println(x);
			lval_del(x);
			mpc_ast_delete(res.output);
		} else {
			mpc_err_print(res.error);
			mpc_err_delete(res.error);
		}

		free(input);
	}

	mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Program);

	return 0;
}
