#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "lex.h"
#include "error.h"

static char curChar = ' ';
static char defaultnextchar(void)
{
	return EOF_CH;
}


static char * tokenNames[] = {
	#define	TOKEN(kind,name)	name,
	#include "tokens.txt"
	#undef	TOKEN
};
Token curToken;
const char * GetTokenName(TokenKind tk){
	return tokenNames[tk];
}

typedef struct {
	TokenKind kind;
	char * name;
}KeywordInfo;

static KeywordInfo keywords[]={
	{TK_INT,"int"},
	{TK_IF, "if"},
	{TK_WHILE,"while"},
	{TK_FOR,"for"},
};

static TokenKind GetkeywordKind(char* id){
	int i = 0;
	for(i=0;i<sizeof(keywords)/sizeof(keywords[0]);i++){
		if(strcmp(id,keywords[i].name)==0){
			return keywords[i].kind;
		}
	}
	return TK_ID;
}

static TokenKind GetTokenKindOfChar(char ch){
	int i = 0;
	for(i=0;i<sizeof(tokenNames) / sizeof(tokenNames[0]);i++){
		if(strlen(tokenNames[i]==1)&&(tokenNames[i][0]==ch)){
			return i;
		}
	}
	return TK_NA;
}
static NEXT_CHAR_FUNC NEXT_CHAR = defaultnextchar;

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
		token.kind = NEXT_CHAR();
	}else if (isalpha(curChar)){
		len = 0;
		do{
			token.value.name[len]= curChar;
			curChar = NEXT_CHAR();
			len++;
		}while (isalnum(curChar) && len < MAX_ID_LEN);
		token.kind = GetkeywordKind(token.value.name);
	}else if(isdigit(curChar)){
		int numVal = 0;
		token.kind = TK_NUM;
		do{
			numVal = numVal*10+(curChar-'0');
			curChar = NEXT_CHAR();
		}while(isdigit(curChar));
		token.value.numVal = numVal;
	}else{
		token.kind = GetTokenKindOfChar(curChar);
		if(token.kind != EOF_CH){
			token.value.name[0]=curChar;
			curChar = NEXT_CHAR();
		}else{
			Error("illegal char \'%x\'.\n",curChar);
			curChar = NEXT_CHAR();
			goto TryAgain;
		}
	}
	return token;
}

void InitLexer(NEXT_CHAR_FUNC next){
	if(next){
		NEXT_CHAR = next;
	}
}

