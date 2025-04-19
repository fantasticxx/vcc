#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"

#define DEFAULT_CTYPE ctype_int

extern void smantic_analysis(ast_node *root);

#endif