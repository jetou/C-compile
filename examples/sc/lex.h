#ifndef LEX_H_
#define LEX_H_
#define MAX_ID_LEN 15

typedef union {
    char name[mZAX_ID_LEN+1]
    int numVal;
}Value;

typedef enum {
    #define TOKEN(Kind,name) Kind,
    #include "token.txt"
    #undef TOKEN
}

typedef struct {
    TokenKind Kind;
    Value value;
}Token;

typedef char (* NEXT_CHAR_FUNC)(void);

#define MAX_ID_LEN 15
#define EOF_CH  ((char)0xff)

extern Token curToken;

#define NEXT_TOKEN do{curToken = GetToken();}while(0)
Token char* GetTokenName(TokenKind tk);
void InitLexer(NEXT_CHAR_FUNC next);

#endif
