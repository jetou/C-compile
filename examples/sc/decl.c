/*************************
*		declaration
 *				int declarator
*///////////////////////////////
#include <stdio.h>
#include "lex.h"
 #include "expr.h"
 #include "error.h"
 #include "decl.h"
 static AstNodePtr DirectDeclarator(void);
 static AstNodePtr PostfixDeclarator(void);
 static AstNodePtr Declarator(void);
/*////////////////////////////////////////////////////////////////////////
   direct-declarator:
 		ID
 		( declarator )
 */
static AstNodePtr DirectDeclarator(void){
	AstNodePtr directDecl = NULL;
	if(curToken.kind == TK_ID){
		directDecl = CreateAstNode(TK_ID,&curToken.value,NULL,NULL);
		NEXT_TOKEN;
	}else if(curToken.kind == TK_LPAREN){
		NEXT_TOKEN;
		directDecl = Declarator();
		Expect(TK_RPAREN);
	}
	return directDecl;
}
/*****************************
   postfix-declarator:
 		direct-declarator
 		postfix-declarator [num]
 		postfix-declarator (void)
 		*/
 static AstNodePtr PostfixDeclarator(void){
 	AstNodePtr decl = DirectDeclarator();
 	while(1){
 		if(curToken.kind == TK_LBRACKET){
 			NEXT_TOKEN;
 			decl = CreateAstNode(TK_ARRAY,&curToken.value,NULL,decl);
 			if(curToken.kind == TK_NUM){
 				decl->value.numVal = curToken.value.numVal;
 				NEXT_TOKEN;
 			}else{
 				decl->value.numVal=0;
 			}
 			Expect(TK_RBRACKET);
 		}else if(curToken.kind == TK_LPAREN){
 			AstNodePtr * param;
 			NEXT_TOKEN;
 			decl = CreateAstNode(TK_FUNCTION,&curToken.value,NULL,decl);
 			param=&(decl->kids[0]);
 			while(curToken.kind == TK_INT){
 				*param = CreateAstNode(TK_INT, &curToken.value,NULL,NULL);
 				param = &((*param)->kids[0]);
 				NEXT_TOKEN;
 				if(curToken.kind==TK_COMMA){
 					NEXT_TOKEN;
 				}
 			}
 			Expect(TK_RPAREN);
 		}else{
 			break;
 		}
 	}
 	return decl;
 }