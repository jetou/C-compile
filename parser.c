#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "parser.h"
//#include "util.h"

ctype_t *ctype_void = &(ctype_t){ CTYPE_VOID, 0 };
ctype_t *ctype_char = &(ctype_t){ CTYPE_CHAR, 1 };
ctype_t *ctype_int = &(ctype_t){ CTYPE_INT, 4 };
ctype_t *ctype_float = &(ctype_t){ CTYPE_FLOAT, 4 };
ctype_t *ctype_double = &(ctype_t){ CTYPE_DOUBLE, 8 };

// lazy
#define _FILE_ parser->lexer->fname
#define _LINE_ parser->lexer->line

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
	if (node->type != NODE_CONSTANT)
		return false;
	if (node->ctype == ctype_int && node->ival == 0)
		return true;
	if ((node->ctype == ctype_float || node->ctype == ctype_double)
		&& node->fval == 0)
		return true;
	return false;
}

static bool is_null(node_t *node)
{
	if (node->type == NODE_CONSTANT && node->ctype == ctype_int && node->ival == 0)
		return true;
	return false;
}

//error message helper functions
static char *type2str(ctype_t *t)
{
	static char *s[] = {
		"void",
		"character",
		"int",
		"float",
		"double",
		"pointer",
		"array"
	};

	assert(t && t->type >= 0 && t->type < 7);
	return s[t->type];
}

static char *punct2str(int punct)
{
	static char *s[] = {
		"++", "+=", "&&", "&=", "||", "|=", "--", "-=", "->",
		"*=", "/=", "%=", "==", "!=", "^=", "<=", "<<", "<<=",
		">=", ">>", ">>="
	};
	
	assert(punct >= PUNCT_INC &&punct <= PUNCT_IRSFT);
	return s[punct - PUNCT_INC];
}

#define NEW_TYPE(ctype, tp, sz)\
	do {\
	(ctype) = calloc(1, sizeof(ctype_t)); \
	(ctype)->type = (tp);\
	(ctype)->size = (sz);\
		}while(0)

static ctype_t *make_ptr(ctype_t *p)
{
	ctype_t *ctype;

	NEW_TYPE(ctype, CTYPE_PTR, 8);
	ctype->ptr = p;
	return ctype;
}

static ctype_t *make_array(ctype_t *p, int len)
{
	ctype_t *ctype;

	NEW_TYPE(ctype, CTYPE_ARRAY, 8);
	ctype->ptr = p;
	ctype->len = len;
	return ctype;
}
//////////////////////node constructors//////////////
#define NEW_NODE(node, tp)\
	do{\
	(node) = calloc(1,sizeof(node_t));\
	(node)->type = tp;\
		}while(0)

static node_t *make_var_decl(ctype_t *ctype, char *varname)
{
	node_t *node;

	NEW_NODE(node, NODE_VAR_DECL);
	node->ctype = ctype;
	node->varname = varname;
	return node;
}

static node_t *make_var_init(node_t *var, node_t *init)
{
	node_t *node;

	NEW_NODE(node, NODE_VAR_INIT);
	node->binary_op = '=';
	node->left = var;
	node->right = init;
	return node;
}

static node_t *make_array_init(node_t *array, vector_t *array_init)
{
	node_t *node;

	NEW_NODE(node, NODE_ARRAY_INIT);
	node->array = array;
	node->array_init = array_init;
	return node;
}

static node_t *make_func_call(ctype_t *ctype, char *func_name, vector_t *args)
{
	node_t *node;

	NEW_NODE(node, NODE_FUNC_CALL);
	node->ctype = ctype->ret;
	node->is_va = ctype->is_va;
	node->func_name = func_name;
	node->params = args;
	return node;
}

static node_t *make_compound_stmt(vector_t *stmts)
{
	node_t *node;

	NEW_NODE(node, NODE_COMPOUND_STMT);
	node->stmts = stmts;
	return node;
}

static node_t *make_cast(ctype_t *ctype, node_t *expr)
{
	node_t *node;

	NEW_NODE(node, NODE_CAST);
	node->ctype = ctype;
	node->expr = expr;
	return node;
}

static node_t *make_arith_conv(ctype_t *ctype, node_t *expr)
{
	node_t *node;

	NEW_NODE(node, NODE_ARITH_CONV);
	node->ctype = ctype;
	node->expr = expr;
	return node;
}

static node_t *make_unary(ctype_t *ctype, int op, node_t *operand)
{
	node_t *node;

	NEW_NODE(node, NODE_UNARY);
	node->ctype = ctype;
	node->unary_op = op;
	node->operand = operand;
	return node;
}

static node_t *make_postfix(ctype_t *ctype, int op, node_t *operand)
{
	node_t *node;

	NEW_NODE(node, NODE_POSTFIX);
	node->ctype = ctype;
	node->unary_op = op;
	node->operand = operand;
	return node;
}

static node_t *make_binary(ctype_t *ctype, int op, node_t *left, node_t *right)
{
	node_t *node;

	NEW_NODE(node, NODE_BINARY);
	node->ctype = ctype;
	node->binary_op = op;
	node->left = left;
	node->right = right;
	return node;
}

static node_t *make_ternary(ctype_t *ctype, node_t *cond, node_t *then, node_t *els)
{
	node_t *node;

	NEW_NODE(node, NODE_TERNARY);
	node->ctype = ctype;
	node->cond = cond;
	node->then = then;
	node->els = els;
	return node;
}

static node_t *make_number(char *s)
{
	node_t *node;
	char *end;

	NEW_NODE(node, NODE_CONSTANT);
	if (strpbrk(s, ".eE")) {
		size_t len = strlen(s);
		char *end;
		if (s[len - 1] == 'f' || s[len - 1] == 'F') {
			node->fval = strlen(s, &end);
			node->ctype = ctype_float;
		}
		else{
			node->fval = strtod(s, &end);
			node->ctype = ctype_double;
		}
	}
	else {
		node->ival = strtoul(s, &end, 0);
		node->ctype = ctype_int;
	}
	if (*end != '\0' && *end != 'f' && *end != 'F')
		error("invalid character \'%c\' \n", *end);

	return node;
}

static node_t *make_char(int c)
{
	node_t *node;

	NEW_NODE(node, NODE_CONSTANT);
	node->ctype = ctype_char;
	node->ival = c;
	return node;
}

static node_t *make_string(char *s)
{
	node_t *node;

	NEW_NODE(node, NODE_STRING);
	node->ctype = make_ptr(ctype_char);
	node->sval = s;
	return node;
}

static node_t *make_if(node_t *cond, node_t *then, node_t *els)
{
	node_t *node;

	NEW_NODE(node, NODE_IF);
	node->cond = cond;
	node->then = then;
	node->els = els;
	return node;
}

static node_t *make_for(node_t *init, node_t *cond, node_t *step, node_t *body)
{
	node_t *node;

	NEW_NODE(node, NODE_FOR);
	node->for_init = init;
	node->for_cond = cond;
	node->for_step = step;
	node->for_body = body;
	return node;
}

static node_t *make_do_while(node_t *cond, node_t *body)
{
	node_t *node;

	NEW_NODE(node, NODE_DO_WHILE);
	node->while_cond = cond;
	node->while_body = body;
	return node;
}

static node_t *make_while(node_t *cond, node_t *body)
{
	node_t *node;

	NEW_NODE(node, NODE_WHILE);
	node->while_cond = cond;
	node->while_body = body;
	return node;
}

static node_t *make_return(ctype_t *ctype, node_t *ret)
{
	node_t *node;

	NEW_NODE(node, NODE_RETURN);
	node->ctype = ctype;
	node->expr = ret;
	return node;
}

//Usual arithmetic conversions
static ctype_t *arith_conv(ctype_t *l, ctype_t *r)
{
	if (l == ctype_double || r == ctype_double)
		return ctype_double;
	else if (l == ctype_float || r == ctype_float)
		return ctype_float;
	return ctype_int;
}

static node_t *conv(ctype_t *ctype, node_t *node)
{
	if (is_same_type(ctype, node->ctype))
		return node;
	return make_arith_conv(ctype, node);
}


//parse function

static node_t *parse_expr(parser_t *parser);
static node_t *parse_cast_expr(parser_t *parser);
static node_t *parse_assign_expr(parser_t *parser);

//Expressions


/* primary-expression:
*      identifier
*      constant
*      string-literal
*      ( expression )
*/

static node_t *parse_primary_expr(parser_t *parser)
{
	token_t *token;
	node_t *primary;

	if (TRY_PUNCT('(')){
		primary = parse_expr(parser);
		EXPECT_PUNCT(')');
		return primary;
	}

	token = NEXT();
	switch (token->type){
	case TK_ID:
		primary = dict_lookup(parser->env, token->sval);
		if (!primary)
			errorf("\'%s\' undeclared in %s:%d\n", token->sval, _FILE_, _LINE_);
		break;
	case TK_NUMBER:
		primary = make_number(token->sval);
		break;
	case TK_CHAR:
		primary = make_char(token->ival);
		break;
	case TK_STRING:
		primary = make_string(token->sval);
		break;
	default:
		errorf("expected expression in %s:%d\n", _FILE_, _LINE_);
		break;
	}
	return primary;
}

/* argument-expression-list:
*      assignment-expression
*      argument-expression-list , assign-expression
*/
static vector_t *parse_arg_expr_list(parser_t *parser, node_t *func)
{
	vector_t *args;
	vector_t *types = func->ctype->param_types;
	size_t i;

	if (types == NULL){
		if (!TRY_PUNCT(')'))
			errorf("too many arguments to function \'%s\' in %s:%d\n", func->func_name, _FILE_, _LINE);
		return NULL;
	}
	if (TRY_PUNCT(')'))
		errorf("too few arguments to function \'%s\' in %s:%d\n", func->func_name, _FILE_, _LINE_);

	args = make_vector();
	//如果实参和形参的类别一样就存进实参列表
	for (i = 0; i < vector_len(types); i++){
		ctype_t *type = vector_get(types, i);
		node_t *arg = parse_assign_expr(parser);
		if (is_arith_type(type) && is_arith_type(arg->ctype))
			arg = conv(type, arg);
		else if (!is_same_type(type, arg->ctype) && !is_null(arg))
			errorf("passing argument %d of \'%s\' makes %s from %s without a cast in %s:%d\n",
			(int)i + 1, func->func_name, type2str(type), type2str(arg->ctype), _FILE_, _LINE_);
		vector_append(args, arg);
		if (i == vector_len(types) - 1 || is_punct(PEEK(), ')'))
			break;
		if (!TRY_PUNCT(','))
			break;
	}

	//refactoring variable argument list
	if (func->ctype->is_va && TRY_PUNCT(',')){
		for (;;){
			node_t *arg = parse_assign_expr(parser);
			//varaiable argument function converts float type arg to double; */
			if (arg->ctype == ctype_float)
				arg = conv(ctype_double, arg);
			vector_append(args, arg);
			if (is_punct(PEEK(), ')'))
				break;
			if (!TRY_PUNCT(','))
				break;
		}
	}
	if (i < vector_len(types) - 1)
		errorf("too few arguments to function \'%s\' in %s:%d\n", func->func_name, _FILE_, _LINE_);
	else if (TRY_PUNCT(','))
		errorf("too many arguments to function \'%s\' in %s:%d\n", func->func_name, _FILE_, _LINE_);
	EXPECT_PUNCT(')');
	return args;
}


/* postfix-expression:
*      primary-expression
*      postfix-expression [ expression ]
*      postfix-expression ( argument-expression-list-opt )
*      postfix-expression . identifier
*      postfix-expression -> identifier
*      postfix-expression ++
*      postfix-expression --
*      ( type-name ) { initializer-list }
*      ( type-name ) { initializer-list , }
*/
static node_t *parse_postfix_expr(parser_t *parser)
{
	node_t *post;
	token_t *token;


	post = parse_primary_expr(parser);
	for (token = NEXT();; token = NEXT()){
		if (is_punct(token, '[')){
			if (!is_ptr(post->ctype))
				errorf("subscripted value is neither array nor pointer in %s:%d\n", _FILE_, _LINE_);
			node_t *expr = parse_expr(parser);
			if (expr->ctype != ctype_int)
				errorf("array subscript is not an integer in %s:%d\n", _FILE_, _LINE_);
			EXPECT_PUNCT(']');
			if (is_array(post->ctype))
				post = make_binary(make_ptr(post->ctype->ptr), '+', post, expr);
			else
				post = make_binary(post->ctype, '+', post, expr);
			post = make_unary(post->ctype->ptr, '*', post);

		}
		else if (is_punct(token, '.'))
			errorf("TODO: struct or union in %s:%d\n", _FILE_, _LINE_);
		else if (is_punct(token, PUNCT_ARROW))
			errorf("TODO: struct or union in %s:%d\n", _FILE_, _LINE_);

		else if (is_punct(token, PUNCT_INC) || is_punct(token))
	}
}