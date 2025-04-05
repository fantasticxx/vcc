#ifndef VCC_H
#define VCC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "symtab.h"
#include "yacc.tab.h"
#include "codegen.h"
#include "ast.h"

extern FILE *obj_f;
extern char *file_name;

extern int yylex();
extern int yyparse(ast_node**);
extern void yyerror(ast_node**, char *, ...);

/* codegen.c */
extern void codegen(ast_node* root);


#endif