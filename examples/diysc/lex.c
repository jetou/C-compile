#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "lex.h"
#include "error.h"

static char curChar = ' ';
static int IsWhiteSpace(char ch){
	if(ch == ' '||ch == '/n'||ch == '/t'||ch == 'r')
		return 1;
	return 0;
}
Token GetToken(void){
	Token token;
	int len = 0;
	memset(&token, 0, sizeof(token));

	while(IsWhiteSpace(curChar)){
		curChar = NEXT_CHAR();
	}
TryAgain:
	if(curChar == EOF_CH){
		
	}
}