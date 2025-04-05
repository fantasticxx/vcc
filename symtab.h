#ifndef SYMTAB_H
#define SYMTAB_H

#include <stdbool.h>
#include "uthash.h"
#include "ast.h"

typedef struct __symbol {
    char *name;
    Ctype *ctype;
    bool mutable;
    int line_no;
    int loc;
    int offset;
    UT_hash_handle hh;
} symbol;

extern symbol *symtab;

extern void st_initialize();
extern void st_insert(char *name, Ctype *ctype, bool mutalbe, int line_no, int loc);
extern symbol* st_lookup(char *name);
extern void st_update(char *name, Ctype *ctype);
extern void print_symbol_table(symbol* symtab);

#endif