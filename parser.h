#ifndef PARSER_H__
#define PARSER_H__

#include <stdbool.h>
#include "lexer.h"
#include "dict.h"

enum {
	CTYPE_VOID,
	CTYPE_CHAR,
	CTYPE_INT,
	CTYPE_FLOAT,
	CTYPE_DOUBLE,
	CTYPE_PTR,
	CTYPE_ARRAY
};

typedef struct ctype_t {
	int type;
	int size;

	struct ctype_t *ptr;

	int len;

	struct ctype_t *ret;

	bool is_va;

	vector_t *param_types;
}ctype_t;

enum {
	NODE_CONSTANT,
	NODE_STRING,
	NODE_POSTFIX,//++ --
	NODE_UNARY,
	NODE_BINARY,
	NODE_TERNARY,// ?;
	NODE_IF,
	NODE_FOR,
	NODE_DO_WHILE,
	NODE_WHILE,
	NODE_FUNC_DECL,
	NODE_FUNC_DEF,
	NODE_FUNC_CALL,
	NODE_VAR_DECL,
	NODE_VAR_INIT,
	NODE_ARRAY_INIT,
	NODE_VAR,
	NODE_COMPOUND_STMT,
	NODE_RETURN,
	NODE_CASE,
	NODE_ARITH_CONV
};

typedef struct node_t {
	int type;
	ctype_t *ctype;
	union {
		long ival;

		struct {
			double fval;
			char *flabel;
		};

		struct {
			char *sval;
			char *slabel;
		};

		struct {
			char *varname;
			union {
				int loffset;
				char *glabel;
			};
		};
	};
};
#endif