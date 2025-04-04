#include <stdlib.h>
#include <ctype.h>
#include "symtab.h"
#include "ast.h"
#include "yacc.tab.h"
#include "list.h"

static Ctype *make_string_type(Ctype *ctype, int len);

Ctype *ctype_bool = &(Ctype){CTYPE_BOOL, 1, NULL};
Ctype *ctype_char = &(Ctype){CTYPE_CHAR, 1, NULL};
Ctype *ctype_int = &(Ctype){CTYPE_INT, 4, NULL};
Ctype *ctype_long = &(Ctype){CTYPE_LONG, 8, NULL};
Ctype *curr_ctype;
bool is_const;

List *strings = &EMPTY_LIST;

void ast_initialize(void)
{
    curr_ctype = NULL;
    is_const = false;
}

ast_node* func(char* fname, ast_node* body)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_FUNC;
    node->ctype = NULL;
    node->fname = fname;
    node->body = body;
    return node;
}

ast_node* block_item_list(ast_node* left, ast_node* right)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_BLOCK_ITEM_LIST;
    node->ctype = NULL;
    node->left = left;
    node->right = right;
    return node;
}

ast_node* decl(ast_node* init_list)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_DECL;
    node->ctype = NULL;
    node->init_list = init_list;
    return node;
}

ast_node* init_declarator(ast_node* left, ast_node* right)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_INIT_DECL;
    node->ctype = curr_ctype;
    node->left = left;
    if (right->ctype == NULL) {
        right->ctype = left->ctype;
    }
    node->right = right;
    return node;
}

ast_node* init_declarator_list(ast_node* left, ast_node* right)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_INIT_DECL_LIST;
    node->ctype = NULL;
    node->left = left;
    node->right = right;
    return node;
}

ast_node* direct_declarator(char* varname)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_DIRECT_DECL;
    node->ctype = curr_ctype;
    node->varname = varname;
    return node;
}

ast_node* if_stmt(ast_node* cond, ast_node* then, ast_node* els)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_IF;
    node->ctype = NULL;
    node->cond = cond;
    node->then = then;
    node->els = els;
    return node;
}

ast_node* while_stmt(ast_node* cond, ast_node* body)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_WHILE;
    node->ctype = NULL;
    node->whilecond = cond;
    node->whilebody = body;
    return node;
}

ast_node* input(char* varname)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_INPUT;
    node->ctype = NULL;
    node->varname = varname;
    return node;
}

ast_node* output(bool lf, ast_node *assign_expr)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_OUTPUT;
    node->ctype = NULL;
    node->lf = lf;
    node->assign_expr = assign_expr;
    return node;
}

ast_node* expr(ast_node* left, ast_node* right)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_EXPR;
    node->ctype = right->ctype;
    node->left = left;
    node->right = right;
    return node;
}

ast_node* assign(ast_node* left, ast_node* right)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_ASSIGN;
    node->ctype = right->ctype;
    node->left = left;
    node->right = right;
    return node;
}

ast_node* logical_or(ast_node* op1, ast_node* op2)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_LOGICAL_OR;
    node->ctype = NULL;
    node->left = op1;
    node->right = op2;
    return node;
}

ast_node* logical_and(ast_node* op1, ast_node* op2)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_LOGICAL_AND;
    node->ctype = NULL;
    node->left = op1;
    node->right = op2;
    return node;
}

ast_node* bitwise_or(ast_node* op1, ast_node* op2)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_BITWISE_OR;
    node->ctype = NULL;
    node->left = op1;
    node->right = op2;
    return node;
}

ast_node* bitwise_xor(ast_node* op1, ast_node* op2)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_BITWISE_XOR;
    node->ctype = NULL;
    node->left = op1;
    node->right = op2;
    return node;
}

ast_node* bitwise_and(ast_node* op1, ast_node* op2)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_BITWISE_AND;
    node->ctype = NULL;
    node->left = op1;
    node->right = op2;
    return node;
}

ast_node* equality(ast_node* op1, ast_node* op2)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_EQ;
    node->ctype = NULL;
    node->left = op1;
    node->right = op2;
    return node;
}

ast_node* not_equality(ast_node* op1, ast_node* op2)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_NE;
    node->ctype = ctype_int;
    node->left = op1;
    node->right = op2;
    return node;
}

ast_node* less_than(ast_node* op1, ast_node* op2)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_LT;
    node->ctype = ctype_int;
    node->left = op1;
    node->right = op2;
    return node;
}

ast_node* greater_than(ast_node* op1, ast_node* op2)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_GT;
    node->ctype = NULL;
    node->left = op1;
    node->right = op2;
    return node;
}

ast_node* less_than_or_equal(ast_node* op1, ast_node* op2)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_LE;
    node->ctype = ctype_int;
    node->left = op1;
    node->right = op2;
    return node;
}

ast_node* greater_than_or_equal(ast_node* op1, ast_node* op2)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_GE;
    node->ctype = NULL;
    node->left = op1;
    node->right = op2;
    return node;
}

ast_node* add(ast_node* op1, ast_node* op2)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_ADD;
    node->ctype = NULL;
    node->left = op1;
    node->right = op2;
    return node;
}

ast_node* sub(ast_node* op1, ast_node* op2)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_SUB;
    node->ctype = NULL;
    node->left = op1;
    node->right = op2;
    return node;
}

ast_node* mul(ast_node* op1, ast_node* op2)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_MUL;
    node->ctype = NULL;
    node->left = op1;
    node->right = op2;
    return node;
}

ast_node* ast_div(ast_node* op1, ast_node* op2)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_DIV;
    node->ctype = NULL;
    node->left = op1;
    node->right = op2;
    return node;
}

ast_node* mod(ast_node* op1, ast_node* op2)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_MOD;
    node->ctype = NULL;
    node->left = op1;
    node->right = op2;
    return node;
}

ast_node* cast_expr(ast_node *operand)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_CAST_EXPR;
    node->ctype = curr_ctype;
    node->operand = operand;
    return node;
}

ast_node* unary_op(int type, ast_node *operand)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = type;
    node->ctype = NULL;
    node->operand = operand;
    return node;
}

ast_node* id(Ctype *ctype, char *name)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_ID;
    node->ctype = ctype;
    node->varname = name;

    return node;
}

ast_node* integer_literal(Ctype* ctype, int val)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_LITERAL;
    node->ctype = ctype;
    node->ival = val;
    return node;
}

static Ctype *make_string_type(Ctype *ctype, int len)
{
    Ctype *r = malloc(sizeof(Ctype));
    r->type = CTYPE_STRING;
    r->ptr = ctype;
    r->size = (len < 0) ? -1 : ctype->size * len;
    r->len = len;
    return r;
}

static int make_string_label()
{
    static int labelseq = 0;
    return labelseq++;
}

ast_node* string(char *str)
{
    ast_node *node = malloc(sizeof(ast_node));
    node->type = AST_STRING;
    node->ctype = make_string_type(ctype_char, strlen(str) + 1);
    node->sval = str;
    node->slabel = make_string_label();
    return node;
}

char *ctype_to_str[] = {
    "char",
    "bool",
    "int",
    "long",
    "string",
};

void print_ast(ast_node* root, int indent)
{
    for(int i = 0; i < indent; i++) {
        printf(" ");
    }
	if (!root) {
		printf("NULL\n");
		return;
	}
    if (root->type == AST_FUNC) {
        printf("AST_FUNC: %s\n", root->fname);
        print_ast(root->body, indent + 4);
    } else if (root->type == AST_BLOCK_ITEM_LIST) {
        printf("AST_BLOCK_ITEM_LIST: \n");
        print_ast(root->left, indent + 4);
        print_ast(root->right, indent + 4);
    } else if (root->type == AST_DECL) {
        if (root->ctype == NULL) {
            printf("AST_DECL: ctype is NULL\n");
        } else {
            printf("AST_DECL: %s<%d>\n", ctype_to_str[root->ctype->type], root->ctype->size);
        }
        print_ast(root->init_list, indent + 4);
    } else if (root->type == AST_INIT_DECL) {
        if (root->ctype == NULL) {
            printf("AST_INIT_DECL: ctype is NULL\n");
        } else {
            printf("AST_INIT_DECL: %s<%d>\n", ctype_to_str[root->ctype->type], root->ctype->size);
        }
        print_ast(root->left, indent + 4);
        print_ast(root->right, indent + 4);
    } else if (root->type == AST_INIT_DECL_LIST) {
        if (root->ctype == NULL) {
            printf("AST_INIT_DECL_LIST: ctype is NULL\n");
        } else {
            printf("AST_INIT_DECL_LIST: %s<%d>\n", ctype_to_str[root->ctype->type], root->ctype->size);
        }
        
        print_ast(root->left, indent + 4);
        print_ast(root->right, indent + 4);
    } else if (root->type == AST_DIRECT_DECL) {
        if (root->ctype == NULL) {
            printf("AST_DIRECT_DECL: %s: ctype is NULL\n", root->varname);
        } else {
            printf("AST_DIRECT_DECL: %s: %s<%d>\n", root->varname, ctype_to_str[root->ctype->type], root->ctype->size);
        }
    } else if(root->type == AST_IF) {
        printf("AST_IF\n");
        print_ast(root->cond, indent + 4);
        print_ast(root->then, indent + 4);
        print_ast(root->els, indent + 4);
    } else if(root->type == AST_WHILE) {
        printf("AST_WHILE\n");
        print_ast(root->cond, indent + 4);
        print_ast(root->body, indent + 4);
    } else if (root->type == AST_INPUT) {
        printf("AST_INTPUT: %s\n", root->varname);
    } else if (root->type == AST_OUTPUT) {
        if (root->lf) {
            printf("PRINTLN\n");
        } else {
            printf("print\n");
        }
        print_ast(root->assign_expr, indent + 4);
    } else if(root->type == AST_EXPR) {
        printf("AST_EXPR\n");
        print_ast(root->left, indent + 4);
        print_ast(root->right, indent + 4);
    } else if (root->type == AST_ASSIGN) {
        printf("AST_ASSIGN\n");
        print_ast(root->left, indent + 4);
        print_ast(root->right, indent + 4);
    } else if (root->type == AST_LITERAL) {
        if (root->ctype->type == CTYPE_CHAR) {
            printf("AST_LITERAL: (char)%c\n", (char)root->ival);
        } else {
            printf("AST_LITERAL: %d\n", root->ival);
        }
    } else if (root->type == AST_STRING) {
        printf("%s\n", root->sval);
    } else if (root->type == AST_ID) {
        symbol *s = NULL;
        HASH_FIND_STR(symtab, root->varname, s);
        printf("AST_ID: %s: %s<%d> %s\n", s->name, ctype_to_str[s->ctype->type], s->ctype->size, (s->mutable ? "mutable" : "immutable"));
    } else if (root->type == AST_UNARY_MIUNS) {
        printf("AST_UNARY_MIUNS\n");
        print_ast(root->operand, indent + 4);
    } else if (root->type == AST_LOGICAL_AND) {
        printf("AST_LOGICAL_AND\n");
        Iter iter = list_iter(root->head);
        for (int i = 0; i < list_len(root->head); i++) {
            print_ast((ast_node *)iter_next(&iter), indent + 4);
        }
    } else if (root->type == AST_LOGICAL_OR) {
        printf("AST_LOGICAL_OR\n");
        Iter iter = list_iter(root->head);
        for (int i = 0; i < list_len(root->head); i++) {
            print_ast((ast_node *)iter_next(&iter), indent + 4);
        }
    } else if (root->type == AST_CAST_EXPR) {
        printf("AST_CAST_EXPR: cast %s<%d> to %s<%d>\n", ctype_to_str[root->operand->ctype->type], root->operand->ctype->size, ctype_to_str[root->ctype->type], root->ctype->size);
        print_ast(root->operand, indent + 4);
    } else {
        switch(root->type) {
        case AST_BITWISE_OR:
            printf("AST_BITWISE_OR\n");
            break;
        case AST_BITWISE_XOR:
            printf("AST_BITWISE_XOR\n");
            break;
        case AST_BITWISE_AND:
            printf("AST_BITWISE_AND\n");
            break;
        case AST_EQ:
            printf("AST_EQ\n");
            break;
        case AST_NE:
            printf("AST_NE\n");
            break;
        case AST_LT:
            printf("AST_LT\n");
            break;
        case AST_GT:
            printf("AST_GT\n");
            break;
        case AST_LE:
            printf("AST_LE\n");
            break;
        case AST_GE:
            printf("AST_GE\n");
            break;
        case AST_ADD:
            printf("AST_ADD\n");
            break;
        case AST_SUB:
            printf("AST_SUB\n");
            break;
        case AST_MUL:
            printf("AST_MUL\n");
            break;
        case AST_DIV:
            printf("AST_DIV\n");
            break;
        case AST_MOD:
            printf("AST_MOD\n");
            break;
        default:
            break;
        }
        print_ast(root->left, indent + 4);
        print_ast(root->right, indent + 4);
    }
}