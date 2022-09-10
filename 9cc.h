#ifndef _9CC_H_
#define _9CC_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>

#define STREAM stdout
#define ERROR stderr

typedef struct Node Node;

/* tokenize.c */
typedef struct Token Token;

typedef enum{
    TK_RESERVED,
    TK_IDENT, // 識別子
    TK_NUM,
    TK_STR,
    TK_EOF
}TokenKind;

struct Token{
    Token* next;
    TokenKind kind;
    int val;
    char* str;
    int len; // トークンの長さ
};

void error(char *fmt, ...);
void error_at(char *loc, char* fmt, ...);
void tokenize(char *path, char* p);

/* type.c */
typedef enum{
    TY_LONG,
    TY_INT,
    TY_CHAR,
    TY_PTR,
    TY_FUNC,
    TY_ARRAY
}TypeKind;

typedef struct Type Type;
struct Type{
    TypeKind kind;
    int size;
    Token *name;
    Type *base;

    /* function type */
    Type *ret_ty;
    Type *next;
    Type *params;
};

extern Type *ty_long;
extern Type *ty_int;
extern Type *ty_char;

Type* pointer_to(Type *base);
Type* array_of(Type *base, int len);
Type* func_type(Type *ret_ty);
Type* copy_type(Type *ty);
bool is_integer(Type *ty);
bool is_ptr(Type* ty);
bool is_func(Type *ty);
void add_type(Node* np);

/* parse.c */
typedef struct Obj Obj;

struct Obj{
    Obj *next;
    Type *ty; // 型情報
    char *name; // 変数の名前
    int offset; // RBPからのオフセット

    char *init_data; // string literal
    bool is_global;

    //function
    Obj *params;
    Obj *locals;
    Node *body;
    int stack_size;
};

typedef enum{
    ND_EXPR_STMT, // expression statement
    ND_STMT_EXPR, // [GNU] statement expression
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_EQ, // ==
    ND_NE, // !=
    ND_LT, // < less than
    ND_LE, // <= less than or equal to
    ND_ASSIGN, // =
    ND_VAR, // variable
    ND_NUM,
    ND_RET, // return
    ND_IF, // if
    ND_WHILE, // while
    ND_FOR, // for
    ND_BLOCK, // {}
    ND_FUNCCALL, // function call
    ND_ADDR, // unary &
    ND_DEREF // unary *
}NodeKind;

struct Node{
    Node* next;
    NodeKind kind;
    Type *ty;

    Node *lhs; // left hand side
    Node *rhs; // right hand side
   
   /* if while for statement */
    Node* cond; 
    Node* then; 
    Node* els; 
    Node* init; 
    Node* inc; 

    char* funcname; // function name
    Node* args; // argments;

    Node* body; // ND_BLOCK or ND_STMT_EXPR
    int val; // ND_NUM用
    Obj* var; // ND_VAR用
};

extern Token *token;

Obj* parse(void);

/* codegen.c */
void codegen(Obj *program);

/* debug.c */
extern FILE* debug;

void check_tokenizer_output(Token* head);
void display_globals(Obj *globals);

#endif