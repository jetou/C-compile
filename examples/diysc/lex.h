#ifndef LEX_H
#define LEX_H

#define MAX_ID_LEN 15

typedef union {
	char name[MAX_ID_LEN];
	int numVal;
}Value;

typedef enum{
	#define TOKEN(kind,name) Kind,
	#include "tokens.txt"
	#undef TOKEN
}TokenKind;

typedef struct{
	TokenKind kind;
	Value value;
}Token;

typedef char (* NEXT_CHAR_FUNC)(void);

