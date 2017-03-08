#ifndef EXPR_H
#define EXPR_H
#include "lex.h"

typedef struct astNode{
	TokenKind op;
	Value value;
	struct astNode *kids[2];
}* AstNodePtr;


