#ifndef _9CC_H_
#define _9CC_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>

#define MAX(x, y) ((x) < (y) ? (y) : (x))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define STREAM stdout
#define ERROR stderr

typedef struct Type Type;
typedef struct Node Node;
typedef struct Member Member;

/* tokenize.c */
typedef struct Token Token;

typedef enum{
    TK_IDENT, // identifier
    TK_PUNCT, // punctuator
    TK_KEYWORD, // keyword
    TK_NUM,
    TK_STR,
    TK_EOF
}TokenKind;

struct Token{
    Token* next;
    TokenKind kind;
    int64_t val;
    Type *ty; // TK_NUM or TK_STR
    char* str;
    int len; // トークンの長さ
};

void error(char *fmt, ...);
void error_at(char *loc, char* fmt, ...);
bool is_ident(void);
bool is_str(void);
bool at_eof(void);
void next_token(void);
bool is_equal(Token *tok, char *op);
bool consume(char* op);
void expect(char* op);
uint64_t expect_number(void);
void tokenize(char *path, char* p);

/* type.c */
typedef enum{
    TY_BOOL,
    TY_LONG,
    TY_INT,
    TY_SHORT,
    TY_CHAR,
    TY_PTR,
    TY_FUNC,
    TY_ARRAY,
    TY_STRUCT,
    TY_UNION,
    TY_VOID,
    TY_ENUM
}TypeKind;

struct Type{
    TypeKind kind;
    int size;
    int align; // alignment
    bool is_unsigned;
    
    Type *base;

    // declaration
    Token *name;
    Token *name_pos;

    /* array */
    int array_len;

    /* struct members */
    Member *members;
    bool is_flexible; // flexible array member or not

    /* function type */
    Type *ret_ty;
    Type *next;
    Type *params;
    bool is_variadic;

};

extern Type *ty_long;
extern Type *ty_int;
extern Type *ty_short;
extern Type *ty_char;
extern Type *ty_void;
extern Type *ty_bool;
extern Type *ty_uchar;
extern Type *ty_ushort;
extern Type *ty_uint;
extern Type *ty_ulong;

Type *enum_type(void);

Type *new_type(TypeKind kind, int size, int align);
Type* pointer_to(Type *base);
Type* array_of(Type *base, int len);
Type* func_type(Type *ret_ty);
Type* copy_type(Type *ty);
Type *struct_type(void);
bool is_integer(Type *ty);
bool is_ptr(Type* ty);
bool is_void(Type *ty);
bool is_func(Type *ty);
bool is_array(Type *ty);
bool is_struct(Type *ty);
bool is_union(Type *ty);
void add_type(Node* np);

/* parse.c */
/* 構造体のメンバ情報 */
struct Member{
    Member *next;
    Type *ty;
    Token *name;
    int offset;
    int idx; // 何番目のメンバか    
    int align; // alignment
};

typedef struct Relocation Relocation;
struct Relocation{
    Relocation *next;
    int offset;
    char *label;
    int64_t addend;
};

typedef struct Obj Obj;

struct Obj{
    Obj *next;
    Type *ty; // 型情報
    char *name; // 変数の名前
    int offset; // RBPからのオフセット
    int align; // alignment
    
    // global variable
    bool is_global;
    char *init_data;
    Relocation *rel;

    //function
    bool is_definition;
    bool is_static;
    Obj *params;
    Obj *locals;
    Obj *va_area; // 可変長引数関数
    Node *body;
    int stack_size;
};

typedef enum{
    ND_NULL_EXPR, // do nothing
    ND_EXPR_STMT, // expression statement
    ND_STMT_EXPR, // [GNU] statement expression
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_MOD, // %
    ND_EQ, // ==
    ND_NE, // !=
    ND_LT, // < less than
    ND_LE, // <= less than or equal to
    ND_NEG, // negative number
    ND_ASSIGN, // =
    ND_COND, // ?:
    ND_VAR, // variable
    ND_NUM,
    ND_RET, // return
    ND_IF, // if
    ND_FOR, // for and while
    ND_DO, // do while
    ND_SWITCH, // switch
    ND_CASE, // case
    ND_BLOCK, // {}
    ND_FUNCCALL, // function call
    ND_ADDR, // unary &
    ND_DEREF, // unary *
    ND_NOT, // !
    ND_BITNOT, // ~
    ND_BITOR, // |
    ND_BITXOR, // ^
    ND_BITAND, // &
    ND_SHL, // shift left
    ND_SHR, // shift right
    ND_LOGAND, // &&
    ND_LOGOR, // ||
    ND_MEMBER, // .
    ND_CAST,
    ND_COMMA, // ,
    ND_GOTO, // goto
    ND_LABEL, // labeled statement
    ND_MEMZERO // zero clear stack variable
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
    char *brk_label;
    char *cont_label;

    Member *member; // 構造体のメンバ

    char* funcname; // function name
    Node* args; // argments;

    // goto and labeled statement
    char *label;
    char *unique_label;
    Node *goto_next;

    // switch-case
    Node *case_next;
    Node *default_case; 

    Node* body; // ND_BLOCK or ND_STMT_EXPR
    int64_t val; // ND_NUM用
    Obj* var; // ND_VAR用
};

extern Token *token;

Obj* parse(void);
Node *new_cast(Node *lhs, Type *ty);

/* codegen.c */
void codegen(Obj *program);
int align_to(int offset, int align);

#endif