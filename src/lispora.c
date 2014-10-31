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

#define LASSERT(args, cond, fmt, ...) \
	if (!(cond)) { \
		lval* err = lval_err(fmt, ##__VA_ARGS__); \
		lval_del(args); \
		return err; \
	}

#define LASSERT_TYPE(func, args, index, expect) \
	LASSERT(args, args->cell[index]->type == expect, \
			"Function '%s' passed incorrect type of argument %i. Got %s, expected %s.", \
			func, index, ltype_name(args->cell[index]->type), ltype_name(expect))

#define LASSERT_NUM_ARGS(func, args, num) \
	LASSERT(args, args->count == num, \
			"Function '%s' passed incorrect number of arguments. Got %i, expected %i.", \
			func, args->count, num)

#define LASSERT_NOT_EMPTY(func, args, index) \
	LASSERT(args, args->cell[index]->count != 0, \
			"Function '%s' passed {} for argument %i.", \
			func, index)

struct lval;
struct lenv;

typedef struct lval lval;
typedef struct lenv lenv;

enum {
	LVAL_NUM,
	LVAL_ERR,
	LVAL_SYM,
	LVAL_STR,
	LVAL_FUN,
	LVAL_SEXPR,
	LVAL_QEXPR
};

typedef lval*(*lbuiltin)(lenv*, lval*);

struct lval {
	int type;

	long num;
	char* err;
	char* sym;
	char* str;

	/* Function */
	lbuiltin builtin;
	lenv* env;
	lval* formals;
	lval* body;

	/* Expression */
	int count;
	lval** cell;
};

struct lenv {
	lenv* par;
	int count;
	char** syms;
	lval** vals;
};

mpc_parser_t* Number;
mpc_parser_t* Symbol;
mpc_parser_t* String;
mpc_parser_t* Comment;
mpc_parser_t* Sexpr;
mpc_parser_t* Expr;
mpc_parser_t* Qexpr;
mpc_parser_t* Program;

char* ltype_name(int t);

lval* lval_num(long num);

lval* lval_err(char* fmt, ...);

lval* lval_sym(char* sym);

lval* lval_str(char* str);

lval* lval_sexpr(void);

lval* lval_qexpr(void);

lval* lval_fun(lbuiltin func);

lval* lval_lambda(lval* formals, lval* body);

void lval_del(lval* v);

lval* lval_copy(lval* v);

lval* lval_add(lval* v, lval* x);

lval* lval_read_num(mpc_ast_t* tree);

lval* lval_read_str(mpc_ast_t* tree);

lval* lval_read(mpc_ast_t* tree);

void lval_print_str(lval* v);

void lval_expr_print(lval* v, char open, char close);

void lval_print(lval* v);

void lval_println(lval* v);

lval* lval_pop(lval* v, int i);

lval* lval_take(lval* v, int i);

lval* lval_eval_sexpr(lenv* e, lval* v);

lval* lval_call(lenv* e, lval* func, lval* args);

lenv* lenv_new(void);

void lenv_del(lenv* e);

lval* lenv_get(lenv* e, lval* key);

lenv* lenv_copy(lenv* e);

void lenv_put(lenv* e, lval* key, lval* v);

void lenv_def(lenv* e, lval* key, lval* v);

lval* builtin_head(lenv* e, lval* args);

lval* builtin_tail(lenv* e, lval* args);

lval* builtin_list(lenv* e, lval* args);

lval* builtin_eval(lenv* e, lval* args);

lval* lval_join(lval* first, lval* second);

lval* builtin_op(lenv* e, lval* args, char* op);

lval* builtin_join(lenv* e, lval* v);

lval* builtin_add(lenv* e, lval* args);

lval* builtin_sub(lenv* e, lval* args);

lval* builtin_mul(lenv* e, lval* args);

lval* builtin_div(lenv* e, lval* args);

lval* builtin_ord(lenv* e, lval* args, char* op);

lval* builtin_gt(lenv* e, lval* args);

lval* builtin_lt(lenv* e, lval* args);

lval* builtin_ge(lenv* e, lval* args);

lval* builtin_le(lenv* e, lval* args);

int lval_eq(lval* first, lval* second);

lval* builtin_cmp(lenv* e, lval* args, char* op);

lval* builtin_eq(lenv* e, lval* args);

lval* builtin_ne(lenv* e, lval* args);

lval* builtin_if(lenv* e, lval* args);

lval* builtin_var(lenv* e, lval* args, char* func);

lval* builtin_def(lenv* e, lval* args);

lval* builtin_put(lenv* e, lval* args);

lval* builtin_import(lenv* e, lval* args);

lval* builtin_print(lenv* e, lval* args);

lval* builtin_error(lenv* e, lval* args);

lval* builtin(lval* args, char* sym);

lval* lval_eval(lenv* e, lval* v);

char* ltype_name(int t) {
	switch(t) {
		case LVAL_FUN: return "Function";
		case LVAL_NUM: return "Number";
		case LVAL_ERR: return "Error";
		case LVAL_SYM: return "Symbol";
		case LVAL_STR: return "String";
		case LVAL_SEXPR: return "S-Expression";
		case LVAL_QEXPR: return "Q-Expression";
		default: return "Unknown";
	}
}

lval* lval_num(long num) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->num = num;
	return v;
}

lval* lval_err(char* fmt, ...) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_ERR;

	va_list va;
	va_start(va, fmt);

	v->err = malloc(512);
	vsnprintf(v->err, 512 - 1, fmt, va);
	v->err = realloc(v->err, strlen(v->err)+1);

	va_end(va);

	return v;
}

lval* lval_sym(char* sym) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SYM;
	v->sym = malloc(strlen(sym) + 1);
	strcpy(v->sym, sym);
	return v;
}

lval* lval_str(char* str) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_STR;
	v->str = malloc(strlen(str) + 1);
	strcpy(v->str, str);
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

lval* lval_fun(lbuiltin func) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_FUN;
	v->builtin = func;
	return v;
}

lval* lval_lambda(lval* formals, lval* body) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_FUN;

	v->builtin = NULL;
	v->env = lenv_new();

	v->formals = formals;
	v->body = body;
	return v;
}

void lval_del(lval* v) {
	switch (v->type) {
		case LVAL_NUM:
			break;
		case LVAL_SYM:
			free(v->sym);
			break;
		case LVAL_STR:
			free(v->str);
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
		case LVAL_FUN:
			if (!v->builtin) {
				lenv_del(v->env);
				lval_del(v->formals);
				lval_del(v->body);
			}
			break;
	}

	free(v);
}

lval* lval_copy(lval* v) {
	lval* copy = malloc(sizeof(lval));
	copy->type = v->type;

	switch (v->type) {
		case LVAL_FUN:
			if (v->builtin) {
				copy->builtin = v->builtin;
			} else {
				copy->builtin = NULL;
				copy->env = lenv_copy(v->env);
				copy->formals = lval_copy(v->formals);
				copy->body = lval_copy(v->body);
			}
			break;
		case LVAL_NUM:
			copy->num = v->num;
			break;
		case LVAL_ERR:
			copy->err = malloc(strlen(v->err) + 1);
			strcpy(copy->err, v->err);
			break;
		case LVAL_SYM:
			copy->sym = malloc(strlen(v->sym) + 1);
			strcpy(copy->sym, v->sym);
			break;
		case LVAL_STR:
			copy->str = malloc(strlen(v->str) + 1);
			strcpy(copy->str, v->str);
			break;

		case LVAL_QEXPR:
		case LVAL_SEXPR:
			copy->count = v->count;
			copy->cell = malloc(sizeof(lval*) * copy->count);
			for (int i = 0; i < copy->count; ++i) {
				copy->cell[i] = lval_copy(v->cell[i]);
			}
			break;
	}

	return copy;
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

lval* lval_read_str(mpc_ast_t* tree) {
	tree->contents[strlen(tree->contents)-1] = '\0';
	char* unescaped = malloc(strlen(tree->contents+1)+1);

	strcpy(unescaped, tree->contents+1);
	unescaped = mpcf_unescape(unescaped);

	lval* str = lval_str(unescaped);
	free(unescaped);
	return str;
}

lval* lval_read(mpc_ast_t* tree) {
	if (strstr(tree->tag, "number")) {
		return lval_read_num(tree);
	}

	if (strstr(tree->tag, "symbol")) {
		return lval_sym(tree->contents);
	}

	if (strstr(tree->tag, "string")) {
		return lval_read_str(tree);
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
		if (!strcmp(tree->children[i]->contents, "(")     ||
				!strcmp(tree->children[i]->contents, ")") ||
				!strcmp(tree->children[i]->contents, "{") ||
				!strcmp(tree->children[i]->contents, "}") ||
				!strcmp(tree->children[i]->tag, "regex")  ||
				strstr(tree->children[i]->tag, "comment")) {
			continue;
		}
		x = lval_add(x, lval_read(tree->children[i]));
	}

	return x;
}

void lval_print_str(lval* v) {
	char* escaped = malloc(strlen(v->str) + 1);
	strcpy(escaped, v->str);
	escaped = mpcf_escape(escaped);
	printf("\"%s\"", escaped);
	free(escaped);
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
		case LVAL_STR:
			lval_print_str(v);
			break;
		case LVAL_SEXPR:
			lval_expr_print(v, '(', ')');
			break;
		case LVAL_QEXPR:
			lval_expr_print(v, '{', '}');
			break;
		case LVAL_FUN:
			if (v->builtin) {
				printf("<function>");
			} else {
				printf("(\\ ");
				lval_print(v->formals);
				putchar(' ');
				lval_print(v->body);
				putchar(')');
			}
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

lval* lval_eval_sexpr(lenv* e, lval* v) {
	for (int i = 0; i < v->count; ++i) {
		v->cell[i] = lval_eval(e, v->cell[i]);
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
	if (first->type != LVAL_FUN) {
		lval* err = lval_err(
				"S-Expression starts with incorrect type. Got %s, expected %s.",
				ltype_name(first->type), ltype_name(LVAL_FUN));
		lval_del(v);
		lval_del(first);
		return err;
	}

	lval* result = lval_call(e, first, v);
	lval_del(first);
	return result;
}

lval* lval_call(lenv* e, lval* func, lval* args) {
	if (func->builtin) {
		return func->builtin(e, args);
	}

	int given = args->count;
	int total = func->formals->count;

	while(args->count) {
		if (func->formals->count == 0) {
			lval_del(args);
			return lval_err("Function passed too amny arguments. Got %i, expected %i.",
					given, total);
		}

		lval* sym = lval_pop(func->formals, 0);

		if (!strcmp(sym->sym, "&")) {
			if (func->formals->count != 1) {
				lval_del(args);
				return lval_err("Function format invalid. Symbol '&' not followed by single symbol");
			}

			lval* next_sym = lval_pop(func->formals, 0);
			lenv_put(func->env, next_sym, builtin_list(e, args));
			lval_del(sym);
			lval_del(next_sym);
			break;
		}

		lval* val = lval_pop(args, 0);
		lenv_put(func->env, sym, val);

		lval_del(sym);
		lval_del(val);
	}

	lval_del(args);

	if (func->formals->count > 0 &&
			!(strcmp(func->formals->cell[0]->sym, "&"))) {
		if (func->formals->count != 2) {
			return lval_err("Function format invalid. Symbol '&' not followed by single symbol");
		}

		lval_del(lval_pop(func->formals, 0));

		lval* sym = lval_pop(func->formals, 0);
		lval* val = lval_qexpr();

		lenv_put(func->env, sym, val);
		lval_del(sym);
		lval_del(val);
	}

	if (func->formals->count == 0) {
		func->env->par = e;
		return builtin_eval(func->env, lval_add(lval_sexpr(), lval_copy(func->body)));
	} else {
		return lval_copy(func);
	}
}

lenv* lenv_new(void) {
	lenv* e = malloc(sizeof(lenv));
	e->par = NULL;
	e->count = 0;
	e->syms = NULL;
	e->vals = NULL;

	return e;
}

void lenv_del(lenv* e) {
	for (int i = 0; i < e->count; ++i) {
		free(e->syms[i]);
		lval_del(e->vals[i]);
	}
	free(e->syms);
	free(e->vals);
	free(e);
}

lval* lenv_get(lenv* e, lval* key) {
	for (int i = 0; i < e->count; ++i) {
		if (!strcmp(e->syms[i], key->sym)) {
			return lval_copy(e->vals[i]);
		}
	}

	if (e->par) {
		return lenv_get(e->par, key);
	} else {
		return lval_err("Unbound symbol: '%s'", key->sym);
	}

}

lenv* lenv_copy(lenv* e) {
	lenv* copy = malloc(sizeof(lenv));
	copy->par = e->par;
	copy->count = e->count;
	copy->syms = malloc(sizeof(char*) * copy->count);
	copy->vals = malloc(sizeof(lval*) * copy->count);

	for (int i = 0; i < e->count; ++i) {
		copy->syms[i] = malloc(strlen(e->syms[i]) + 1);
		strcpy(copy->syms[i], e->syms[i]);
		copy->vals[i] = lval_copy(e->vals[i]);
	}

	return copy;
}

void lenv_def(lenv* e, lval* key, lval* v) {
	while (e->par) {
		e = e->par;
	}

	lenv_put(e, key, v);
}

void lenv_put(lenv* e, lval* key, lval* v) {
	for (int i = 0; i < e->count; ++i) {
		if (!strcmp(e->syms[i], key->sym)) {
			lval_del(e->vals[i]);
			e->vals[i] = lval_copy(v);
			return;
		}
	}

	e->count++;
	e->vals = realloc(e->vals, sizeof(lval*) * e->count);
	e->syms = realloc(e->syms, sizeof(char*) * e->count);

	e->vals[e->count-1] = lval_copy(v);
	e->syms[e->count-1] = malloc(strlen(key->sym) + 1);
	strcpy(e->syms[e->count-1], key->sym);
}

lval* builtin_head(lenv* e, lval* args) {
	LASSERT_NUM_ARGS("head", args, 1);
	LASSERT_TYPE("head", args, 0, LVAL_QEXPR);
	LASSERT_NOT_EMPTY("head", args, 0);

	lval* head = lval_take(args, 0);
	while(head->count > 1) {
		lval_del(lval_pop(head, 1));
	}

	return head;
}

lval* builtin_tail(lenv* e, lval* args) {
	LASSERT_NUM_ARGS("tail", args, 1);
	LASSERT_TYPE("tail", args, 0, LVAL_QEXPR);
	LASSERT_NOT_EMPTY("tail", args, 0);

	lval* tail = lval_take(args, 0);
	lval_del(lval_pop(tail, 0));
	return tail;
}

lval* builtin_list(lenv* e, lval* args) {
	args->type = LVAL_QEXPR;
	return args;
}

lval* builtin_eval(lenv* e, lval* args) {
	LASSERT_NUM_ARGS("eval", args, 1);
	LASSERT_TYPE("eval", args, 0, LVAL_QEXPR);

	lval* x = lval_take(args, 0);
	x->type = LVAL_SEXPR;
	return lval_eval(e, x);
}

lval* lval_join(lval* first, lval* second) {
	while (second->count) {
		first = lval_add(first, lval_pop(second, 0));
	}

	lval_del(second);
	return first;
}

lval* builtin_join(lenv* e, lval* v) {
	for (int i = 0; i < v->count; ++i) {
		LASSERT_TYPE("join", v, i, LVAL_QEXPR);
	}

	lval* res = lval_pop(v, 0);
	while(v->count) {
		res = lval_join(res, lval_pop(v, 0));
	}

	lval_del(v);
	return res;
}

lval* builtin_lambda(lenv* e, lval* args) {
	LASSERT_NUM_ARGS("\\", args, 2);
	LASSERT_TYPE("\\", args, 0, LVAL_QEXPR);
	LASSERT_TYPE("\\", args, 1, LVAL_QEXPR);

	// Check if the first Q-Expression contains only symbols
	for (int i = 0; i < args->cell[0]->count; ++i) {
		LASSERT(args, (args->cell[0]->cell[i]->type == LVAL_SYM),
				"Cannot define non-symbol. Got %s, expected %s",
				ltype_name(args->cell[0]->cell[i]->type), ltype_name(LVAL_SYM));
	}

	lval* formals = lval_pop(args, 0);
	lval* body = lval_pop(args, 0);
	lval_del(args);

	return lval_lambda(formals, body);

}

lval* builtin_op(lenv* e, lval* args, char* op) {
	for (int i = 0; i < args->count; ++i) {
		LASSERT_TYPE(op, args, i, LVAL_NUM);
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

lval* builtin_add(lenv* e, lval* args) {
	return builtin_op(e, args, "+");
}

lval* builtin_sub(lenv* e, lval* args) {
	return builtin_op(e, args, "-");
}

lval* builtin_mul(lenv* e, lval* args) {
	return builtin_op(e, args, "*");
}

lval* builtin_div(lenv* e, lval* args) {
	return builtin_op(e, args, "/");
}

lval* builtin_ord(lenv* env, lval* args, char* op) {
	LASSERT_NUM_ARGS(op, args, 2);
	LASSERT_TYPE(op, args, 0, LVAL_NUM);
	LASSERT_TYPE(op, args, 1, LVAL_NUM);

	int res;
	if (!strcmp(op, ">")) {
		res = (args->cell[0]->num > args->cell[1]->num);
	} else if (!strcmp(op, "<")) {
		res = (args->cell[0]->num < args->cell[1]->num);
	} else if (!strcmp(op, ">=")) {
		res = (args->cell[0]->num >= args->cell[1]->num);
	} else if (!strcmp(op, "<=")) {
		res = (args->cell[0]->num <= args->cell[1]->num);
	}

	lval_del(args);
	return lval_num(res);
}

lval* builtin_gt(lenv* e, lval* args) {
	return builtin_ord(e, args, ">");
}

lval* builtin_lt(lenv* e, lval* args) {
	return builtin_ord(e, args, "<");
}

lval* builtin_ge(lenv* e, lval* args) {
	return builtin_ord(e, args, ">=");
}

lval* builtin_le(lenv* e, lval* args) {
	return builtin_ord(e, args, "<=");
}

int lval_eq(lval* first, lval* second) {
	if (first->type != second->type) {
		return 0;
	}

	switch (first->type) {
		case LVAL_NUM:
			return first->num == second->num;
		case LVAL_ERR:
			return !strcmp(first->err, second->err);
		case LVAL_SYM:
			return !strcmp(first->sym, second->sym);
		case LVAL_STR:
			return !strcmp(first->str, second->str);
		case LVAL_FUN:
			if (first->builtin || second->builtin) {
				return first->builtin == second->builtin;
			} else {
				return lval_eq(first->formals, second->formals) &&
					lval_eq(first->body, second->body);
			}
		case LVAL_QEXPR:
		case LVAL_SEXPR:
			if (first->count != second->count) {
				return 0;
			}
			for (int i = 0; i < first->count; ++i) {
				if (!lval_eq(first->cell[i], second->cell[i])) {
					return 0;
				}
			}
			return 1;
	}

	return 0;
}

lval* builtin_cmp(lenv* e, lval* args, char* op) {
	LASSERT_NUM_ARGS(op, args, 2);
	int res;
	if (!strcmp(op, "==")) {
		res = lval_eq(args->cell[0], args->cell[1]);
	} else if (!strcmp(op, "!=")) {
		res = !lval_eq(args->cell[0], args->cell[1]);
	}

	lval_del(args);
	return lval_num(res);
}

lval* builtin_eq(lenv* e, lval* args) {
	return builtin_cmp(e, args, "==");
}

lval* builtin_ne(lenv* e, lval* args) {
	return builtin_cmp(e, args, "!=");
}

lval* builtin_if(lenv* e, lval* args) {
	LASSERT_NUM_ARGS("if", args, 3);
	LASSERT_TYPE("if", args, 0, LVAL_NUM);
	LASSERT_TYPE("if", args, 1, LVAL_QEXPR);
	LASSERT_TYPE("if", args, 2, LVAL_QEXPR);

	lval* x;
	args->cell[1]->type = LVAL_SEXPR;
	args->cell[2]->type = LVAL_SEXPR;

	if (args->cell[0]->num) {
		x = lval_eval(e, lval_pop(args, 1));
	} else {
		x = lval_eval(e, lval_pop(args, 2));
	}

	lval_del(args);
	return x;
}

lval* builtin_var(lenv* e, lval* args, char* func) {
	LASSERT_TYPE(func, args, 0, LVAL_QEXPR);

	lval* syms = args->cell[0];
	for (int i = 0; i < syms->count; ++i) {
		LASSERT(args, (syms->cell[i]->type == LVAL_SYM),
				"Function '%s' cannot define non-symbol. Got %s, expected %s",
				func, ltype_name(syms->cell[i]->type), ltype_name(LVAL_SYM));
	}

	LASSERT(args, (syms->count == args->count-1),
			"Function '%s' got too many arguments for symbols. Got %i, expected %i.",
			func, syms->count, args->count-1);

	for (int i = 0; i < syms->count; ++i) {
		if (!strcmp(func, "def")) {
			lenv_def(e, syms->cell[i], args->cell[i+1]);
		} else if (!strcmp(func, "=")) {
			lenv_put(e, syms->cell[i], args->cell[i+1]);
		}
	}

	lval_del(args);
	return lval_sexpr();
}

lval* builtin_def(lenv* e, lval* args) {
	return builtin_var(e, args, "def");
}

lval* builtin_put(lenv* e, lval* args) {
	return builtin_var(e, args, "=");
}

lval* builtin_print(lenv* e, lval* args) {
	for (int i = 0; i < args->count; ++i) {
		lval_print(args->cell[i]);
		putchar(' ');
	}

	putchar('\n');
	lval_del(args);

	return lval_sexpr();
}

lval* builtin_error(lenv* e, lval* args) {
	LASSERT_NUM_ARGS("error", args, 1);
	LASSERT_TYPE("error", args, 0, LVAL_STR);

	lval* err = lval_err(args->cell[0]->str);

	lval_del(args);

	return err;
}

lval* builtin_import(lenv* e, lval* args) {
	LASSERT_NUM_ARGS("import", args, 1);
	LASSERT_TYPE("import", args, 0, LVAL_STR);


	mpc_result_t r;
	if (mpc_parse_contents(args->cell[0]->str, Program, &r)) {
		lval* expr = lval_read(r.output);
		mpc_ast_delete(r.output);

		while (expr->count) {
			lval* curr = lval_eval(e, lval_pop(expr, 0));
			if (curr->type == LVAL_ERR) {
				lval_println(curr);
			}
			lval_del(curr);
		}

		lval_del(expr);
		lval_del(args);

		return lval_sexpr();
	} else {
		char* err_msg = mpc_err_string(r.error);
		mpc_err_delete(r.error);

		lval* err = lval_err("Could not import file '%s'", err_msg);
		free(err_msg);
		lval_del(args);

		return err;
	}
}

void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {
	lval* key = lval_sym(name);
	lval* val = lval_fun(func);

	lenv_put(e, key, val);

	lval_del(key);
	lval_del(val);
}

void lenv_add_builtins(lenv* e) {
	/* List functions */
	lenv_add_builtin(e, "list", builtin_list);
	lenv_add_builtin(e, "head", builtin_head);
	lenv_add_builtin(e, "tail", builtin_tail);
	lenv_add_builtin(e, "eval", builtin_eval);
	lenv_add_builtin(e, "join", builtin_join);

	/* Mathematical functions */
	lenv_add_builtin(e, "+", builtin_add);
	lenv_add_builtin(e, "-", builtin_sub);
	lenv_add_builtin(e, "*", builtin_mul);
	lenv_add_builtin(e, "/", builtin_div);

	/* Comparison */
	lenv_add_builtin(e, "==", builtin_eq);
	lenv_add_builtin(e, "!=", builtin_ne);
	lenv_add_builtin(e, ">", builtin_gt);
	lenv_add_builtin(e, ">=", builtin_ge);
	lenv_add_builtin(e, "<", builtin_lt);
	lenv_add_builtin(e, "<=", builtin_le);

	/* Other */
	lenv_add_builtin(e, "def", builtin_def);
	lenv_add_builtin(e, "=", builtin_def);
	lenv_add_builtin(e, "\\", builtin_lambda);
	lenv_add_builtin(e, "if", builtin_if);
	lenv_add_builtin(e, "import", builtin_import);
	lenv_add_builtin(e, "print", builtin_print);
	lenv_add_builtin(e, "error", builtin_error);
}

lval* lval_eval(lenv* e, lval* v) {
	if (v->type == LVAL_SYM) {
		lval* sym_val = lenv_get(e, v);
		lval_del(v);
		return sym_val;
	}

	if (v->type == LVAL_SEXPR) {
		return lval_eval_sexpr(e, v);
	}
	return v;
}

int main(int argc, char** argv) {

	Number  = mpc_new("number");
	Symbol  = mpc_new("symbol");
	String  = mpc_new("string");
	Comment = mpc_new("comment");
	Sexpr   = mpc_new("sexpr");
	Expr    = mpc_new("expr");
	Qexpr   = mpc_new("qexpr");
	Program = mpc_new("program");

	mpca_lang(MPCA_LANG_DEFAULT,
		"                                                          \
			number  : /-?[0-9]+/ ;                                 \
			symbol  : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;           \
			string  : /\"(\\\\.|[^\"])*\"/ ;                       \
			comment : /;[^\\r\\n]*/ ;                              \
			sexpr   : '(' <expr>* ')' ;                            \
			qexpr   : '{' <expr>* '}' ;                            \
			expr    : <number>  | <symbol> | <string>              \
					| <comment> | <sexpr>  | <qexpr>;              \
			program : /^/ <expr>* /$/ ;                            \
		",
		Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Program);

	lenv* env = lenv_new();
	lenv_add_builtins(env);

	if (argc == 1) {
		puts("Lispora version 0.1.0.0.0");
		puts("Press Ctrl-C for exit.");

		while (1) {
			char* input = readline("lispora> ");
			add_history(input);

			mpc_result_t res;

			// Parse input from the stdin as a Program and copy the result to res.
			// If it is successful, return 1, otherwise 0.
			int success = mpc_parse("<stdin>", input, Program, &res);

			if (success) {
				lval* x = lval_eval(env, lval_read(res.output));
				lval_println(x);
				lval_del(x);
				mpc_ast_delete(res.output);
			} else {
				mpc_err_print(res.error);
				mpc_err_delete(res.error);
			}

			free(input);
		}
	}

	if (argc >= 2) {
		for (int i = 1; i < argc; ++i) {
			lval* args = lval_add(lval_sexpr(), lval_str(argv[i]));
			lval* curr = builtin_import(env, args);

			if (curr->type == LVAL_ERR) {
				lval_println(curr);
			}

			lval_del(curr);
		}
	}

	lenv_del(env);
	mpc_cleanup(8, Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Program);

	return 0;
}
