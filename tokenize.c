#include "9cc.h"

static char *current_path;
static char *current_input;

/* エラー表示用の関数 */
void error(char *fmt, ...){
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

/* エラー表示用の関数 */
void error_at(char *loc, char* fmt, ...){
    va_list ap;
    va_start(ap, fmt);
    char *start = loc;
    /* locが含まれる行の開始地点と終了地点を取得 */
    while(current_input < start && start[-1] != '\n'){
        start--;
    }
    char *end = loc;
    while(*end != '\n'){
        end++;
    }

    int line_num = 1;

    /* pointerをstartまで移動。\nが出てくるたびにline_numを更新 */
    for(char *p = current_input; p < start; p++){
        if(*p == '\n'){
            line_num++;
        }
    }

    /* エラーメッセージを表示 */
    int indent = fprintf(stderr, "%s:%d: ", current_path, line_num);
    fprintf(ERROR, "%.*s\n", (int)(end - start), start);

    int pos = loc - start + indent; // ポインタの引き算は要素数を返す。
    fprintf(ERROR, "%*s", pos, " "); // pos個の空白を表示
    fprintf(ERROR, "^ ");
    vfprintf(ERROR, fmt, ap); 
    fprintf(ERROR, "\n");
    exit(1);
}

/* Token操作用の関数 */
bool is_ident(void){
    return token -> kind == TK_IDENT;
}

bool is_str(void){
    return token -> kind == TK_STR;
}

/* TK_EOF用。トークンがEOFかどうかを返す。*/
bool at_eof(void){
    return token -> kind == TK_EOF;
}

/* 次のトークンを読む。(tokenを更新するのはこの関数のみ) */
void next_token(void){
    token = token -> next;
}

/* トークンの記号が期待したもののときtrue。それ以外の時false */
bool is_equal(Token *tok, char *op){
    if(strlen(op) != tok -> len || strncmp(op, tok -> str, tok -> len))
        return false;
    return true;
}

/* トークンが期待した記号のときはトークンを読み進めて真を返す。それ以外のときは偽を返す。*/
bool consume(char* op){
    if(is_equal(token, op)){
        next_token();
        return true;
    }
    return false;
}

/* トークンが期待した記号の時はトークンを読み進めて真を返す。それ以外の時にエラー */
void expect(char* op){
    if(!is_equal(token, op))
        error_at(token->str, "%sではありません\n", op);
    next_token();
}

/* TK_NUM用。トークンが数値の時にトークンを読み進めて数値を返す。それ以外の時エラー */
uint64_t expect_number(void){
    if(token -> kind != TK_NUM)
        error_at(token -> str, "数ではありません\n");
    uint64_t val = token -> val;
    next_token();
    return val;
}

/* 新しいtokenを作成する */
static Token* new_token(TokenKind kind, char *start, char *end){
    Token* tok = calloc(1, sizeof(Token));
    tok -> kind = kind;
    tok -> str = start;
    tok -> len = end - start;
    return tok;
}

/* 文字列を比較。memcmpは成功すると0を返す。 */
static bool startswith(char* p1, char* p2){
    return strncmp(p1, p2, strlen(p2)) == 0;
}

static bool is_keyword(Token *tok){
    static char* kw[] = {"return", "if", "else", "while", "for", "int", "sizeof", "char", "struct", "union", "long", "short", "void", "typedef", "_Bool"};
    for(int i =0; i < sizeof(kw) / sizeof(*kw); i++){
        if(is_equal(tok, kw[i])){
            return true;
        }
    }
    return false;
}

/* 区切り文字だった場合長さを返す */
static int read_puct(char *p){
    static char* kw[] = {"==", "!=", "<=", ">=", "->"};
    for(int i =0; i < sizeof(kw) / sizeof(*kw); i++){
        if(startswith(p, kw[i])){
            return strlen(kw[i]);
        }
    }
    // +-*/()<>;={},&[].
    return ispunct(*p) ? 1 : 0;
}

/* int_to_charのような識別子を受け取れる様にするため、取り合えずTK_IDENTにしておいて後で種類を編集する */
static void convert_keywords(Token *head){
    for(Token *tok = head; tok -> kind != TK_EOF; tok = tok -> next){
        if(is_keyword(tok)){
            tok -> kind = TK_KEYWORD;
        }
    }
}

static Token *read_char_literal(char *start){
    char *p = start + 1;
    char c = *p;
    char *end = strchr(p, '\'');
    if(!end){
        error_at(start, "unclosed char literal\n");
    }
    Token *tok = new_token(TK_NUM, start, end + 1);
    tok -> val = c;
    return tok;
}

/* 入力文字列をトークナイズしてそれを返す */
void tokenize(char *path, char* p){
    Token head; /* これは無駄になるがスタック領域なのでオーバーヘッドは0に等しい */
    Token* cur = &head;

    current_path = path;
    current_input = p;

    while(*p){
        /* is~関数は偽のときに0を、真の時に0以外を返す。*/
        /* spaceだった場合は無視。 */
        if(isspace(*p)){
            p++;
            continue;
        }

        /* 行コメントをスキップ */
        if(strncmp(p, "//", 2) == 0){
            p += 2;
            while(*p != '\n'){
                p++;
            }
            continue;
        }

        /* ブロックコメントをスキップ */
        if(strncmp(p, "/*", 2) == 0) {
            char *q = strstr(p + 2, "*/");
            if (!q)
                error_at(p, "コメントが閉じられていません");
            p = q + 2;
            continue;
        }

        /* 数値だった場合 */
        if(isdigit(*p)){
            cur = cur -> next = new_token(TK_NUM, p, p);
            char *q = p;
            cur -> val = strtol(p, &p, 10);
            cur -> len = p - q; /* 長さを記録 */
            continue;
        }

        if(*p == '\''){
            cur = cur -> next = read_char_literal(p);
            p += cur -> len;
            continue;
        }

        /* 文字列リテラルの場合 */
        if(*p == '"'){
            p++; // '"'を読み飛ばす
            cur = cur -> next = new_token(TK_STR, p, p);
            char *q = p;
            for(; *p != '"'; p++){
                if(*p == '\\'){
                    p++; // '\'の次にある文字を飛ばす。(一字的な措置。)
                }
            }
            cur -> len  = p - q;
            p++; // '"'を読み飛ばす
            continue;
        }

        /* ローカル変数の場合(数字が使用される可能性もあることに注意。) */
        if(isalnum(*p) || *p == '_'){
            cur = cur -> next = new_token(TK_IDENT, p, p);
            char *q = p;
            while(isalnum(*p) || *p == '_'){
                p++;
            }
            cur -> len = p - q; /* 長さを記録 */
            
            continue;
        }
        
        /* puctuators */
        int punct_len = read_puct(p);
        if(punct_len){
            cur = cur -> next = new_token(TK_PUNCT, p, p + punct_len);
            p += punct_len;
            continue;
        }

        /* それ以外 */
        error_at(p, "トークナイズできません\n");
    }

    /* 終了を表すトークンを作成 */
    cur = cur -> next = new_token(TK_EOF, p, p);

    convert_keywords(head.next);

    /* トークンの先頭へのポインタをセット */
    token = head.next;
}


