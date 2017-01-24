#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"
#include "dict.h"
#include "buffer.h"



#define PUTC(buffer, c)\
	do{\
	buffer_push(buffer, &(c), sizeof(char));\
		}while (0)

#define SET_STRING(buffer, s)\
	do {\
		(s) = malloc(sizeof(char) * (buffer->top + 1)); \
		memcpy(s, buffer->stack, buffer->top);\
		s[buffer->top] = '\0'; \
	}while(0)


// io
static int get_c(lexer_t *lexer)
{
	int c = getc(lexer->fp);

	if (c == '\n'){
		lexer->line++;
		lexer->prev_column = lexer->column;
		lexer->column = 0;
	}
	else
		lexer->column++;
	return c;
}

static void unget_c(int c, lexer_t *lexer)
{
	if (c == '\n'){
		lexer->line--;
		lexer->column = lexer->prev_column;
	}
	else
		lexer->column--;
	ungetc(c, lexer->fp);
}

static bool expect_c(int c, lexer_t *lexer)
{
	int c1 = get_c(lexer);

	if (c1 == c)
		return true;
	else{
		unget_c(c1, lexer);
		return false;
	}
}

static dict_t *kw;

int is_keyword(const char *s)
{
	int ret;

	if (!kw){
		kw = make_dict(NULL);
		dict_insert(kw, "void", (void *)KW_VOID, true);
		dict_insert(kw, "char", (void *)KW_CHAR, true);
		dict_insert(kw, "int", (void *)KW_INT, true);
		dict_insert(kw, "float", (void *)KW_FLOAT, true);
		dict_insert(kw, "double", (void *)KW_DOUBLE, true);
		dict_insert(kw, "for", (void *)KW_FOR, true);
		dict_insert(kw, "do", (void *)KW_DO, true);
		dict_insert(kw, "while", (void *)KW_WHILE, true);
		dict_insert(kw, "if", (void *)KW_IF, true);
		dict_insert(kw, "else", (void *)KW_ELSE, true);
		dict_insert(kw, "return", (void *)KW_RETURN, true);
	}
	
	ret = (long)dict_lookup(kw, s);
	return ret ? ret : 0;
}

#define NEW_TOKEN(token, tp)\
	do {\
		(token) = malloc(sizeof(*(token))); \
		(token)->type = (tp);\
			}while(0)

static token_t *make_number(char *s)
{
	token_t *token;

	NEW_TOKEN(token, TK_NUMBER);
	token->sval = s;
	return token;
}

static token_t *make_char(int c)
{
	token_t *token;
	NEW_TOKEN(token, TK_CHAR);
	token->ival = c;
	return token;
}

static token_t *make_string(char *s)
{
	token_t *token;

	NEW_TOKEN(token, TK_STRING);
	token->sval = s;
	return token;
}

static token_t *make_keyword(int type)
{
	token_t *token;

	NEW_TOKEN(token, TK_KEYWORD);
	token->ival = type;
	return token;
}

static token_t *make_id(char *s)
{
	token_t *token;

	NEW_TOKEN(token, TK_ID);
	token->ival = s;
	return token;
}

static token_t *make_punct(int c)
{
	token_t *token;
	
	NEW_TOKEN(token, TK_PUNCT);
	token->ival = c;
	return token;
}
////////////////////////////////////////////////////
static token_t *make_punct_2(lexer_t *lexer, int punct_type, int expect1, int punct_type1)
{
	if (expect_c(expect1, lexer))
		return make_punct(punct_type1);
	else
		return make_punct(punct_type);
}

static token_t *make_punct_3(lexer_t *lexer, int punct_type, int expect1, int punct_type1, int expect2, int punct_type2)
{
	int c = get_c(lexer);

	if (c == expect1)
		return make_punct(punct_type1);
	else if (c == expect2)
		return make_punct(punct_type2);
	else{
		unget_c(c, lexer);
		return make_punct(punct_type);
	}
}
//lex
static void lex_whitespace(lexer_t *lexer)
{
	int c;

	while (isspace(c = get_c(lexer)))
		;
	unget_c(c, lexer);
}

static int lex_escape(int c)
{
	switch (c){
	case'\'':
	case '\"':
	case '\?':
	case '\\':
		return c;
	case 'a':
		return '\a';
	case 'b':
		return '\b';
	case 'f':
		return '\f';
	case 'n':
		return '\n';
	case 'r':
		return '\r';
	case 't':
		return '\t';
	case 'v':
		return '\v';
	default:
		return -1;
	}
}

static token_t *lex_char(lexer_t *lexer)
{
	int c = get_c(lexer);

	if (c == '\\'){
		int temp = get_c(lexer);
		c = lex_escape(temp);
		if (c==-1)
			errorf("unknown escape sequence: \'\\%c\' in %s:%d:%d\n", temp, lexer->fname, lexer->line, lexer->column);
	}
	if (get_c(lexer) != '\'')
		errorf("missing terminating \' character in %s:%d:%d\n", lexer->fname, lexer->line, lexer->column);
	return make_char(c);
}

static token_t *lex_string(lexer_t *lexer)
{
	int c;
	char *s;
	buffer_t *string = make_buffer();

	for (c = get_c(lexer);; c= get_c(lexer))
		switch (c) {
		case '\"':
			SET_STRING(string, s);
			free_buffer(string);
			return make_string(s);

		case '\\':{
			int temp = get_c(lexer);
			c = lex_escape(temp);
			if (c == 1)
				errorf("unknown escape sequence \'\\%c\' in %s:%d:%d\n", temp, lexer->fname, lexer->line, lexer->column);
			PUTC(string, c);
			break;
		}

		default:
			if (c == EOF)
				error("missing terminating \" character  in %s:%d:%d\n", lexer->fname, lexer->line, lexer->column);
			PUTC(string, c);
			break;
	}
}

static token_t *lex_id(lexer_t *lexer, int c)
{
	buffer_t *id = make_buffer();
	char *s;

	assert(isalpha(c) || c == '_');
	PUTC(id, c);
	for (c = get_c(lexer); isalnum(c) || c == '_'; c = get_c(lexer))
		PUTC(id, c);
	unget_c(c, lexer);

	SET_STRING(id, s);
	free_buffer(id);
	c = is_keyword(s);
	if (c){
		free(s);
		return make_key_word(c);
	}
	else
		return make_id(s);
}

static token_t *lex_number(lexer_t *lexer, char c)
{
	buffer_t *num = make_buffer();
	char *s;

	assert(isdigit(c));
	PUTC(num, c);
	for (c = get_c(lexer); isdigit(c); c = get_c(lexer))
		PUTC(num, c);
	if (c == 'f')

	
}