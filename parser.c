#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "parser.h"
//#include "util.h"

ctype_t *ctype_void = &(ctype_t){ CTYPE_VOID, 0 };
ctype_t *ctype_char = &(ctype_t){ CTYPE_CHAR, 1 };
ctype_t *ctype_int = &(ctype_t){ CTYPE_INT, 4 };
ctype_t *ctype_float = &(ctype_t){ CTYPE_FLOAT, 4 };
ctype_t *ctype_double = &(ctype_t){ CTYPE_DOUBEL, 8 };

// lazy
#define _FILE_ parser->lexer->fname;
#define _LINE_ parser->lexer->line;

//i/o function
#define NEXT()(get_token(parser->lexer))
#define PEEK()(peek_token(parser->lexer))
#define UNGET(token)(unget_token(token, parser->lexer))
#define EXPECT_PUNCT(punct)\
	do{\
		token_t *token = NEXT();\
		if(token->type!=TK_PUNCT||token->ival != (punct))\
			errorf("expected \'%c\' in %s:%d\n", punct, _FILE_, _LINE_); \
			}while(0)
#define TRY_PUNCT(punct)\
	(is_punct(PEEK(), punct)? (NEXT(), true):false)

#define EXPECT_KW(keyword)\
	do{\
		token_t *token = NEXT();\
		if(token->type!=TK_KEYWORD||token->ival!=keyword)\
		errorf("expected keyword \'%d\' in %s:%d\n", keyword, _FILE_, _LINE_); \
		}while(0)
#define TRY_KW(keyword)\
	(is_keyword(PEEK(),keyword) ? (NEXT(), true) : false)


//type check function
static bool is_punct(token_t *token, int punct)
{
	if (token->type == TK_PUNCT && token->ival == punct)
		return true;
	return false;
}

static bool is_keyword(token_t *token, int keyword)
{
	if (token->type == TK_KEYWORD && token->ival == keyword)
		return true;
	return false;
}

static bool is_type(token_t *token)
{
	if (token->type != TK_KEYWORD)
		return false;
	switch (token->ival){
	case KW_VOID:
	case KW_CHAR:
	case KW_INT:
	case KW_FLOAT:
	case KW_DOUBLE:
		return true;
	default:
		return false;
	}
}


static int is_assign_op(token_t *token)
{
	if (token->type != TK_PUNCT)
		return false;
	switch (token->ival){
	case '=':
		return '=';
	case PUNCT_IMUL:
		return '*';
	case PUNCT_IDIV:
		return '/';
	case PUNCT_IMOD:
		return '%';
	case PUNCT_IADD:
		return '+';
	case PUNCT_ISUB:
		return '-';
	case PUNCT_ILSFT:
		return PUNCT_LSFT;
	case PUNCT_IRSFT:
		return PUNCT_RSFT;
	case PUNCT_IAND:
		return '&';
	case PUNCT_IXOR:
		return '^';
	case PUNCT_IOR:
		return '|';

	default:
		break;
	}
	return 0;
}

bool is_array(ctype_t *ctype)
{
	if (ctype->type == CTYPE_ARRAY)
		return true;
	return false;
}

static bool is_lvalue(node_t *node)
{
	if ((node->type == NODE_VAR_DECL && !is_array(node->ctype))
		|| (node->type == NODE_UNARY && node->unary_op == '*'))
		return true;
	return false;
}

bool is_ptr(ctype_t *ctype)
{
	if (ctype->type == CTYPE_PTR || ctype->type == CTYPE_ARRAY)
		return true;
	return false;
}

static bool is_same_type(ctype_t *t, ctype_t *p)
{
	if (t == p)
		return true;

	if (is_ptr(t) && is_ptr(p)
		&& (t->ptr == ctype_void || p->ptr == ctype_void || is_same_type(t->ptr, p->ptr)))
		return true;
	return false;
}

static bool is_arith_type(ctype_t *type)
{
	if (type == ctype_int || type == ctype_float || type == ctype_double)
		return true;
	return false;
}

static bool is_zero(node_t *node)
{

}