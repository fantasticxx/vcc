#include "yacc.tab.h"
#include "ast.h"

static Ctype* promote_type(Ctype* type)
{
    if (type->size < 4) {
        return ctype_int;
    }
    return type;
}

static Ctype* get_result_type(Ctype* left, Ctype* right)
{
    Ctype *ltype = promote_type(left);
    Ctype *rtype = promote_type(right);
    return ltype->size >= rtype->size ? ltype : rtype;
}

Ctype* type_check(ast_node* root)
{
    if (!root) {
        yyerror(NULL, "null node");
        exit(1);
    }
    if (root->type == AST_LITERAL) {
        return ctype_int;
    }
    if (root->type == AST_ID) {
        symbol* s = st_lookup(root->varname);
        if (s == NULL) {
            yyerror(NULL, "undefined variable: %s", root->varname);
            exit(1);
        }
        return s->ctype;
    }
    Ctype *left_type = type_check(root->left);
    Ctype *right_type = type_check(root->right);
    if (root->type == AST_ADD || root->type == AST_SUB || root->type == AST_MUL || root->type == AST_DIV) {
        root->ctype = get_result_type(left_type, right_type);
        return root->ctype;
    }
    
}

void smantic_analysis(ast_node* root)
{
    type_check(root);
}