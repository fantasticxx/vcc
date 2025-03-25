#include "semantic.h"
#include "yacc.tab.h"
#include "ast.h"

// static Ctype* promote_type(Ctype* type)
// {
//     if (type->size < 4) {
//         return ctype_int;
//     }
//     return type;
// }

// static Ctype* get_result_type(Ctype* left, Ctype* right)
// {
//     Ctype *ltype = promote_type(left);
//     Ctype *rtype = promote_type(right);
//     return ltype->size >= rtype->size ? ltype : rtype;
// }

// Ctype* type_check(ast_node* root)
// {
//     if (!root) {
//         yyerror(NULL, "null node");
//         exit(1);
//     }
//     if (root->type == AST_LITERAL) {
//         return ctype_int;
//     }
//     if (root->type == AST_ID) {
//         symbol* s = st_lookup(root->varname);
//         if (s == NULL) {
//             yyerror(NULL, "undefined variable: %s", root->varname);
//             exit(1);
//         }
//         return s->ctype;
//     }
//     Ctype *left_type = type_check(root->left);
//     Ctype *right_type = type_check(root->right);
//     if (root->type == AST_ADD || root->type == AST_SUB || root->type == AST_MUL || root->type == AST_DIV) {
//         root->ctype = get_result_type(left_type, right_type);
//         return root->ctype;
//     }
    
// }

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
    // type_check(root);
    find_logical_operator(root);
}