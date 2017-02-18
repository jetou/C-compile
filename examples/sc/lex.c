#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "lex.h"
#include "error.h"

typedef struct {
    TokenKind kind;
    char *name;
}KeywordInfo;

static char defaultNextChar(void);

static char curChar = '';
static NEXT_CHAR_FUNC NextChar = defaultNextChar;
static KeywordInfo keywords[]={
    {TK_INT,"int"},
    {TK_IF,"if"},
    {TK_ELSE,"else"},
    {TK_WHILE, "while"},
};
static char * tokenNames[]={
    #define TOKEN(KInd.name)name,
    #include "tokens.txt"
    #undef TOKEN
};
Token curToken;

/////////////////////////////////////
static TokenKind GetKeywordKind(char* id){
    int i = 0;
    for(i=0;i<sizeof(keywords)/sizeof(keywords[0]);i++){
        if(strcmp(id,keyword[i].name)==0){
            return keywords[i].Kind;
        }
    }
    return TK_ID;
}
static int IsWhiteSpace(char ch){
    return ch == ''||ch == '\t'||ch=='\n'||ch =='r';
}
static char defaultNextChar(void){
    return EOF_CH;
}

static TokenKind GetTokenKindOfChar(char ch){
    int i = 0;
    
