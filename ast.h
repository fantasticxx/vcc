#ifndef AST_H
#define AST_H

#include <stdbool.h>
#include "list.h"


enum {
    AST_LOGICAL_OR,
    AST_LOGICAL_AND,
    AST_BITWISE_OR,
    AST_BITWISE_XOR,
    AST_BITWISE_AND,
    AST_EQ,
    AST_NE,
    AST_LT,
    AST_GT,
    AST_LE,
    AST_GE,
    AST_ADD,
    AST_SUB,
    AST_MUL,
    AST_DIV,
    AST_MOD,
    AST_ID,
    AST_LITERAL,
    AST_STRING,
    AST_FUNC,
    AST_DECL,
    AST_ARRAY_INIT,
    AST_IF,
    AST_WHILE,
    AST_COMPOUND_STMT,
    AST_TYPE_SPEC,
    AST_INIT_DECL,
    AST_INIT_DECL_LIST,
    AST_BLOCK_ITEM_LIST,
    AST_DIRECT_DECL,
    AST_EXPR,
    AST_ASSIGN,
    AST_CAST_EXPR,
    AST_UNARY_MIUNS,
    AST_INPUT,
    AST_OUTPUT,
};

enum {
    CTYPE_CHAR,
    CTYPE_BOOL,
    CTYPE_INT,
    CTYPE_LONG,
    CTYPE_STRING,
};

typedef struct __Ctype {
    int type;
    int size;
    struct __Ctype *ptr; /* pointer or array */
    int len;             /* array length */
} Ctype;

typedef struct __ast_node {
    int type;
    Ctype *ctype;
    union {
        int ival;

        struct {
            char *sval;
            char *slabl;
        };

        struct {
            char *varname;
            struct {
                int loff;
                char *glabel;
            };
        };

        struct {
            struct __ast_node *left;
            struct __ast_node *right;
        };

        struct {
            struct __ast_node *operand;
        };

        struct {
            char *fname;
            struct __ast_node *body;
            List *localvars;
        };

        struct {
            struct __ast_node *init_list;
        };

        struct {
            struct __ast_node *declvar;
            struct __ast_node *declinit;
        };

        struct {
            struct __ast_node *cond;
            struct __ast_node *then;
            struct __ast_node *els;
        };

        struct {
            struct __ast_node *whilecond;
            struct __ast_node *whilebody;
        };
        
        struct {
            bool ln;
            struct __ast_node *assign_expr;
        };

        List *head;
    };
} ast_node;

extern Ctype *ctype_bool;
extern Ctype *ctype_char;
extern Ctype *ctype_int;
extern Ctype *ctype_long;
extern Ctype *curr_ctype;
extern bool is_const;

extern void ast_initialize(void);
extern ast_node *func(char* fname, ast_node* body);
extern ast_node *block_item_list(ast_node* left, ast_node* right);
extern ast_node *decl(ast_node* init_list);
extern ast_node *init_declarator_list(ast_node* left, ast_node* right);
extern ast_node *init_declarator(ast_node* left, ast_node* right);
extern ast_node *direct_declarator(char* varname);
extern ast_node *if_stmt(ast_node* cond, ast_node* then, ast_node* els);
extern ast_node *while_stmt(ast_node* cond, ast_node* body);
extern ast_node *input(char* varname);
extern ast_node *output(bool ln, ast_node* assign_stmt);
extern ast_node *expr(ast_node* left, ast_node* right);
extern ast_node *assign(ast_node* left, ast_node* right);
extern ast_node *logical_or(ast_node* op1, ast_node* op2);
extern ast_node *logical_and(ast_node* op1, ast_node* op2);
extern ast_node *bitwise_or(ast_node* op1, ast_node* op2);
extern ast_node *bitwise_xor(ast_node* op1, ast_node* op2);
extern ast_node *bitwise_and(ast_node* op1, ast_node* op2);
extern ast_node *equality(ast_node* op1, ast_node* op2);
extern ast_node *not_equality(ast_node* op1, ast_node* op2);
extern ast_node *less_than(ast_node* op1, ast_node* op2);
extern ast_node *greater_than(ast_node* op1, ast_node* op2);
extern ast_node *less_than_or_equal(ast_node* op1, ast_node* op2);
extern ast_node *greater_than_or_equal(ast_node* op1, ast_node* op2);
extern ast_node *add(ast_node* op1, ast_node* op2);
extern ast_node *sub(ast_node* op1, ast_node* op2);
extern ast_node *mul(ast_node* op1, ast_node* op2);
extern ast_node *ast_div(ast_node* op1, ast_node* op2);
extern ast_node *mod(ast_node* op1, ast_node* op2);
extern ast_node *cast_expr(Ctype* ctype, ast_node* operand);
extern ast_node *unary_op(int type, ast_node* operand);
extern ast_node *id(Ctype* ctype, char* name);
extern ast_node *integer_literal(Ctype* ctype, int val);
extern ast_node *string(char* str);

void print_ast(ast_node* root, int indent);

#endif