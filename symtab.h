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

#endif