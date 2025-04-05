#include "semantic.h"
#include "yacc.tab.h"
#include "ast.h"
#include "symtab.h"

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

static void check_mutable(ast_node* root)
{
    symbol *s = st_lookup(root->varname);
    if (s == NULL) {
        fprintf(stderr, "error: unknown variable %s\n", root->varname);
        exit(1);
    }
    if (!s->mutable) {
        fprintf(stderr, "error: cannot assign to variable %s is immutable\n", root->varname);
        exit(1);
    }
}

static void check_is_zero(ast_node* root)
{
    if (root->type == AST_UNARY_MIUNS || root->type == AST_CAST_EXPR) {
        check_is_zero(root->operand);
    } else if (root->type == AST_LITERAL && root->ival == 0) {
        fprintf(stderr, "warning: division by zero is undefined\n");
    }
}

static Ctype* check_expression(ast_node* root)
{
    if (root->type == AST_LITERAL || root->type == AST_STRING || root->type == AST_ID) {
        return root->ctype;
    } else if (root->type == AST_EXPR) {
        check_expression(root->left);
        return check_expression(root->right);
    } else if (root->type == AST_ASSIGN) {
        Ctype *ltype = check_expression(root->left);
        Ctype *rtype = check_expression(root->right);
        check_mutable(root->right);
        if (ltype->type != rtype->type) {
            if (ltype->type == CTYPE_STRING) {
                fprintf(stderr, "warning: incompatible type conversion: string to %s\n", ctype_to_str[rtype->type]);
            }
            if (rtype->type == CTYPE_STRING) {
                fprintf(stderr, "warning: incompatible type conversion: %s to string\n", ctype_to_str[ltype->type]);
            }
        }
        root->ctype = rtype;
        return root->ctype;
    } else if (root->type == AST_UNARY_MIUNS) {
        Ctype *ctype = check_expression(root->operand);
        if (ctype->type == CTYPE_STRING) {
            fprintf(stderr, "error: unary minus on string\n");
            exit(1);
        }
        root->ctype = promote_type(ctype);
        return root->ctype;
    } else if (root->type == AST_CAST_EXPR) {
        Ctype *ctype = check_expression(root->operand);
        if (root->ctype->size < ctype->size) {
            fprintf(stderr, "warning: cast to smaller type: %s<%d> from %s<%d>\n",
                    ctype_to_str[root->ctype->type], root->ctype->size,
                    ctype_to_str[ctype->type], ctype->size);
        }
        return root->ctype;
    }

    Ctype *ltype = check_expression(root->left);
    Ctype *rtype = check_expression(root->right);
    if (ltype->type == CTYPE_STRING || rtype->type == CTYPE_STRING) {
        if (root->type == AST_MUL || root->type == AST_DIV ||
            root->type == AST_MOD || root->type == AST_BITWISE_AND ||
            root->type == AST_BITWISE_OR || root->type == AST_BITWISE_XOR) {
            fprintf(stderr, "error: invalid operands to binary expression\n");
            exit(1);
        }
        fprintf(stderr, "warning: string operands in binary expression\n");
    }
    if (root->type == AST_DIV) {
        check_is_zero(root->right);
    }
    root->ctype = get_result_type(ltype, rtype);
    return root->ctype;
}

static void check_variable_declaration(ast_node* root)
{
    if (root->type == AST_INIT_DECL_LIST) {
        check_variable_declaration(root->left);
        check_variable_declaration(root->right);
    } else if (root->type == AST_INIT_DECL) {
        Ctype *ltype = check_expression(root->left);
        if (root->right->ctype == NULL) {
            root->right->ctype = ltype ? ltype : DEFAULT_CTYPE;
        }
        check_variable_declaration(root->right);
        if (ltype != root->right->ctype) {
            if (ltype == ctype_string) {
                fprintf(stderr, "warning: incompatible type conversion: string to %s\n", ctype_to_str[root->right->ctype->type]);
            }
            if (root->right->ctype == ctype_string) {
                fprintf(stderr, "warning: incompatible type conversion: %s to string\n", ctype_to_str[ltype->type]);
            }
        }
    } else if (root->type == AST_DIRECT_DECL) {
        symbol *s = st_lookup(root->varname);
        if (s->ctype == NULL) {
            if (root->ctype == NULL) {
                root->ctype = DEFAULT_CTYPE;
            }
            st_update(root->varname, root->ctype);
        }
    }
}

static void check_type(ast_node* root)
{
    if (!root) {
        return;
    }
    if (root->type == AST_FUNC) {
        return check_type(root->body);
    } else if (root->type == AST_BLOCK_ITEM_LIST) {
        check_type(root->left);
        check_type(root->right);
    } else if (root->type == AST_DECL) {
        check_variable_declaration(root->init_list);
    } else if (root->type == AST_EXPR) {
        check_expression(root->left);
        check_expression(root->right);
    } else if (root->type == AST_IF) {
        check_expression(root->cond);
        check_type(root->then);
        check_type(root->els);
    } else if (root->type == AST_WHILE) {
        check_expression(root->whilecond);
        check_type(root->whilebody);
    } else if (root->type == AST_OUTPUT) {
        check_expression(root->assign_expr);
    } else {

    }
}

void flatten_logical_and_ast_to_list(ast_node* root, List* list)
{
    if (root->type == AST_LOGICAL_AND) {
        flatten_logical_and_ast_to_list(root->left, list);
        flatten_logical_and_ast_to_list(root->right, list);
    } else {
        list_push(list, root);
    }
}

void flatten_logical_or_ast_to_list(ast_node* root, List* list)
{
    if (root->type == AST_LOGICAL_OR) {
        flatten_logical_or_ast_to_list(root->left, list);
        flatten_logical_or_ast_to_list(root->right, list);
    } else {
        list_push(list, root);
    }
}

void find_logical_operator(ast_node* root)
{
    if(!root ||
        root->type == AST_DIRECT_DECL ||
        root->type == AST_INPUT ||
        root->type == AST_LITERAL ||
        root->type == AST_STRING ||
        root->type == AST_ID) {
        return;
    }
    if (root->type == AST_LOGICAL_OR) {
        List *list = make_list();
        flatten_logical_or_ast_to_list(root, list);
        root->head = list;
        Iter iter = list_iter(list);
        for (int i = 0; i < list_len(list); i++) {
            find_logical_operator((ast_node *)iter_next(&iter));
        }
    } else if (root->type == AST_LOGICAL_AND) {
        List *list = make_list();
        flatten_logical_and_ast_to_list(root, list);
        root->head = list;
        Iter iter = list_iter(list);
        for (int i = 0; i < list_len(list); i++) {
            find_logical_operator((ast_node *)iter_next(&iter));
        }
    } else if (root->type == AST_FUNC) {
        find_logical_operator(root->body);
    } else if (root->type == AST_DECL) {
        find_logical_operator(root->init_list);
    } else if (root->type == AST_OUTPUT) {
        find_logical_operator(root->assign_expr);
    } else if (root->type == AST_IF) {
        find_logical_operator(root->cond);
        find_logical_operator(root->then);
        find_logical_operator(root->els);
    } else if (root->type == AST_WHILE) {
        find_logical_operator(root->whilecond);
        find_logical_operator(root->whilebody);
    } else {
        find_logical_operator(root->left);
        find_logical_operator(root->right);
    }
}

void smantic_analysis(ast_node* root)
{
    check_type(root);
    find_logical_operator(root);
}