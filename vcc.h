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

extern int yylex();
extern int yyparse(ast_node**);
extern void yyerror(ast_node**, char *, ...);

/* symtab.c */
extern void st_initialize();
extern void st_insert(char *name, Ctype *ctype, bool mutalbe, int line_no, int loc);
extern symbol* st_lookup(char *name);

/* codegen.c */
extern void codegen(ast_node* root);
#endif