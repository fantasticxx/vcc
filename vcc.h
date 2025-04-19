#ifndef VCC_H
#define VCC_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "codegen.h"
#include "symtab.h"
#include "yacc.tab.h"

extern FILE *obj_f;
extern char *file_name;
extern int error_count;

extern int yylex();
extern void yyerror(ast_node **, char *, ...);
extern int yyparse(ast_node **);  //

#endif