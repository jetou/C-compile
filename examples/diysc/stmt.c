#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "stmt.h"
#include "decl.h"
#include "error.h"

static TokenKind prefixOfStmt[] = {
	TK_ID, TK_IF, IK_WHILE, TK_LBARCE, TK_INT
};

static int isPrefixOfStatement(TokenKind tk){
	int i = 0;
	for(i = 0; i < sizeof(prefixOfStmt)/sizeof(prefixOfStmt[0]);i++){

	}

}

static AstStmtNodePtr CreateStmtNode(TokenKind op){
	AstStmtNodePtr pNode = (AstStmtNodePtr)malloc(sizeof(struct astStmtNode));
	memset(pNode,0,sizeof(*pNode));
	return pNode;
}

static AstNodePtr CreateLabelNode(void){
	Value value;
	memset(&value,0,sizeof(Value));
	snprintf(value.name,MAX_ID_LEN,"label_%d",NewLabel());
	return CreateAstNode(TK_LABEL, &value, NULL, NULL);
}

static AstStmtNodePtr ExpressionStatment(void){
	if(curToken.kind == TK_ID){
		//id = Expression
		AstStmtNodePtr assign = CreateStmtNode(TK_ASSIGN);
		assign->kids[0] = CreateAstNode(TK_ID,&curToken.value,NULL,NULL);
		NEXT_TOKEN;
		if(curToken.kind == TK_ASSIGN){
			NEXT_TOKEN;
			assign->expr = Expression();
		}else{
			Error("stmt:  '=' expected.\n");
		}
		Expect(TK_SEMICOLON);
		return assign;
	}else if(IS_PREFIX_OF_DECL(curToken.kind)){
		//declaration
		AstStmtNodePtr decl = CreateStmtNode(TK_DECLARATION);
		decl->expr = Declaration();
		Expect(TK_SEMICOLON);
		return decl;
	}else{
		Error("stmt: id expected.\n");
		return NULL;
	}
}
/************************************************************
*********************************************************/
static AstStmtNodePtr IfStatement(void){
	AstStmtNodePtr ifStmt = NULL;

	ifStmt = CreateStmtNode(TK_IF);
	Expect(TK_IF);
	Expect(TK_LPAREN);
	ifStmt->expr = Expression();
	Expect(TK_RPAREN);

	ifStmt->thenStmt = Statement();
	ifStmt->kids[0] = CreateLabelNode();
	if(curToken.kind == TK_ELSE){
		NEXT_TOKEN;
		ifStmt->elseStmt = Statement();
		ifStmt->kids[1] = CreateLabelNode();
	}
	return ifStmt;
}

static AstStmtNodePtr WhileStatement(void){
	AstStmtNodePtr whileStmt = NULL;
	whileStmt = CreateStmtNode(TK_WHILE);

	whileStmt->kids[0] = CreateLabelNode();
	Expect(TK_WHILE);
	Expect(TK_LPAREN);
	whileStmt->expr = Expression();
	Expect(TK_RPAREN);
	whileStmt->thenStmt = Statement();

	whileStmt->kids[1] = CreateLabelNode();
	return whileStmt;
}

AstStmtNodePtr CompoundStatement(void){
	AstStmtNodePtr comStmt;
	AstStmtNodePtr * pStmt;
	Value value;

	comStmt = CreateStmtNode(TK_COMPOUND);
	pStmt = &(comStmt->next);

	Expect(TK_LBRACE);//{
	while(isPrefixOfStatement(curToken.kind)){
		*pStmt = Statement();
		pStmt = &((*pStmt)->next);
	}
	Expect(TK_RBRACE);
	return comStmt;
}
AstStmtNodePtr Statement(void){
	switch(curToken.kind){
	case TK_IF:
		return IfStatement();
	case TK_WHILE:
		return WhileStatement();
	case TK_LBRACE:
		return CompoundStatement();
	case TK_ID:
	case TK_INT:
		return ExpressionStatment();
	}
	assert(0);
	return NULL;
}


void VisitStatementNode(AstStmtNodePtr stmt){
	if(!stmt){
		return;
	}
	switch(stmt->op){
	case TK_IF:
		VisitArithmeticNode(stmt->expr);
		if(stmt->kids[0] && stmt->kids[1]){
			printf("\tif(!%s) goto %s \n",stmt->expr->value.name,stmt->kids[0]->value.name);
			VisitStatementNode(stmt->thenStmt);
			printf("\tgoto %s \n",stmt->kids[1]->value.name);
			printf("%s:\n",stmt->kids[0]->value.name);
			VisitStatementNode(stmt->elseStmt);
			printf("%s:\n",stmt->kids[1]->value.name);
		}else{
			printf("\tif(!%s) goto %s \n",stmt->expr->value.name,stmt->kids[0]->value.name);
			VisitStatementNode(stmt->thenStmt);
			printf("%s:\n",stmt->kids[0]->value.name);
		}
		break;
	case TK_WHILE:
		printf("%s:\n",stmt->kids[0]->value.name);
		VisitArithmeticNode(stmt->expr);
		printf("\tif(!%s) goto %s \n",stmt->expr->value.name,stmt->kids[1]->value.name);
		VisitStatementNode(stmt->thenStmt);
		printf("\tgoto %s \n",stmt->kids[0]->value.name);
		printf("%s:\n",stmt->kids[1]->value.name);
		break;
	case TK_COMPOUND:
		while(stmt->next){
			VisitStatementNode(stmt->next);
			stmt = stmt->next;
		}
		break;
	case TK_ASSIGN:
		VisitArithmeticNode(stmt->expr);
		if(stmt->kids[0] && stmt->expr){
			if(stmt->expr->op == TK_NUM){
				printf("\t%s = %d \n",stmt->kids[0]->value.name,stmt->expr->value.numVal);
			}else{
				printf("\t%s = %s \n",stmt->kids[0]->value.name,stmt->expr->value.name);
			}
		}
		break;
	case TK_DECLARATION:	//declaration
		VisitDeclarationNode(stmt->expr);
		break;
	}
}
