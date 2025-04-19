#include "symtab.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

symbol *symtab;

void st_initialize()
{
    symtab = NULL;
}

void st_insert(char *name, Ctype *ctype, bool mutalbe, int line_no, int loc)
{
    symbol *s = malloc(sizeof(symbol));
    s->name = name;
    s->ctype = ctype;
    s->mutable = mutalbe;
    s->line_no = line_no;
    s->loc = loc;
    HASH_ADD_STR(symtab, name, s);
}

symbol *st_lookup(char *name)
{
    symbol *s = NULL;
    HASH_FIND_STR(symtab, name, s);
    return s;
}

void st_update(char *name, Ctype *ctype)
{
    symbol *s = st_lookup(name);
    if (s) {
        s->ctype = ctype;
    }
}

/* Procedure printSymTab prints a formatted
 * listing of the symbol table contents
 * to the listing file
 */

void generate_internal_name() {}

void which_type() {}

void which_value() {}

void print_symbol_table(symbol *symtab)
{
    for (symbol *cur = symtab; cur; cur = cur->hh.next) {
        printf("Name: %s, Type: %s, Mutable: %s, Line No: %d, Loc: %d\n",
               (cur->name ? cur->name : "NULL"),
               (cur->ctype ? ctype_to_str[cur->ctype->type] : "NULL"),
               cur->mutable ? "true" : "false", cur->line_no, cur->loc);
    }
}