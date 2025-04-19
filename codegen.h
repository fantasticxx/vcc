#ifndef CODEGEN_H
#define CODEGEN_H

#include "yacc.tab.h"

#define STRING_RESERVED_BYTES 100

extern void codegen(ast_node *root);

#endif