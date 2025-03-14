#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "ast.h"
#include "symtab.h"

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

symbol* st_lookup(char *name)
{
	symbol *s = NULL;
	HASH_FIND_STR(symtab, name, s);
	return s;
} 

void st_update(char *name, long val)
{
	
}

/* Procedure printSymTab prints a formatted 
 * listing of the symbol table contents 
 * to the listing file
 */

void generate_internal_name()
{
	
}

void which_type() {

}

void which_value() {

}