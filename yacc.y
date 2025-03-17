%code requires {
#include "vcc.h"
#include "ast.h"

extern int yylineno;

static Ctype *ctype_bool = &(Ctype){CTYPE_BOOL, 1, NULL};
static Ctype *ctype_char = &(Ctype){CTYPE_CHAR, 1, NULL};
static Ctype *ctype_int = &(Ctype){CTYPE_INT, 4, NULL};
static Ctype *ctype_long = &(Ctype){CTYPE_LONG, 8, NULL};
static Ctype *ctype_const = &(Ctype){CTYPE_CONST, 8, NULL};

}

%parse-param {ast_node **root}

%union {
	int ival;
	char cval;
	char *sval;
	ast_node *ast;
	Ctype *ctype;
}

%token 	<ctype> BOOL CHAR CONST INT STRING
%token 	READ PRINT PRINTLN
%token 	IF ELSE WHILE
%token 	<sval> MAIN ID
%token	<ival> BOOL_VAL NUMBER
%token  <cval> CHAR_CONST
%token  <sval> STRING_CONST
%token	<ival> INC_OP DEC_OP LE_OP GE_OP EQ_OP NE_OP AND_OP OR_OP 
%token	MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN SUB_ASSIGN
%token	AND_ASSIGN XOR_ASSIGN OR_ASSIGN

%type <ast>  program prog_body compound_statement block_item_list block_item declaration
			 init_declarator_list init_declarator declarator direct_declarator initializer
			 statement expression_statement selection_statement iteration_statement
			 io_statement expression assignment_expression conditional_expression
			 logical_or_expression logical_and_expression inclusive_or_expression
			 exclusive_or_expression and_expression equality_expression relational_expression
			 additive_expression multiplicative_expression cast_expression
			 unary_expression postfix_expression primary_expression


%type <ctype> type_specifier type_qualifier
%type <ival> unary_operator
%type <sval> prog_hdr

%left '+' '-'
%left '*' '/' '%' 
%left '<' '>' LE_OP GE_OP

%nonassoc UMINUS

%precedence ')'
%precedence ELSE

%start program

%%

program: prog_hdr prog_body								{ *root = func($1, $2); }
	   ;

prog_hdr: MAIN '(' ')'									{ $$ = $1; }
		;

prog_body: compound_statement							{ $$ = $1; }
		 ;

compound_statement: '{' '}'								{ $$ = NULL; }
				  | '{' block_item_list '}'				{ $$ = $2; }
				  ; 

block_item_list: block_item								{ $$ = $1; }
			   | block_item_list block_item				{ $$ = block_item_list($1, $2); }
			   ;

block_item: declaration									{ $$ = $1; }
		  | statement									{ $$ = $1; }
		  ;

declaration: declaration_specifiers init_declarator_list ';' 	{ $$ = decl($2); curr_ctype = NULL; is_const = false; }
		   ;

declaration_specifiers: type_specifier
					  | type_specifier declaration_specifiers
					  | type_qualifier
					  | type_qualifier declaration_specifiers

type_specifier: INT                         				{ curr_ctype = ctype_int; }
		 	  | CHAR                       					{ curr_ctype = ctype_char; }
		 	  | BOOL                       					{ curr_ctype = ctype_bool; }
		 	  | STRING                     					{ curr_ctype = ctype_long; }
		 	  ;

type_qualifier: CONST					   					{ is_const = true; }
		 	  ;

init_declarator_list: init_declarator							{ $$ = $1; }
					| init_declarator_list ',' init_declarator 	{ $$ = init_declarator_list($1, $3); }
					;

init_declarator: declarator									{ $$ = $1;}
			   | declarator '=' initializer					{ $$ = init_declarator($3, $1); }
			   ;

declarator: direct_declarator								{ $$ = $1; }
		  ;

direct_declarator: ID										{ 
															  if (st_lookup($1)) {
																yyerror(root, "redefinition of %s\n", $1);
																YYABORT;
															  }
															  st_insert($1, curr_ctype, !is_const, @1.first_line, @1.first_column);
															  $$ = direct_declarator($1);
															}
				 | '(' ID ')'								{
															  if (st_lookup($2)) {
																yyerror(root, "redefinition of %s\n", $2);
																YYABORT;
															  }
															  st_insert($2, curr_ctype, !is_const, @2.first_line, @2.first_column);
															  $$ = direct_declarator($2);
															}
				 ;

initializer: assignment_expression												{ $$ = $1; }
		   ;

statement: compound_statement													{ $$ = $1; }
		 | expression_statement													{ $$ = $1; }	
		 | selection_statement													{ $$ = $1; }
		 | iteration_statement													{ $$ = $1; }
		 | io_statement															{ $$ = $1; }
		 ;

expression_statement: ';'														{ $$ = NULL; }
					| expression ';'											{ $$ = $1; }
			   		;

selection_statement: IF '(' expression ')' statement							{ $$ = if_stmt($3, $5, NULL); }
				   | IF '(' expression ')' statement ELSE statement				{ $$ = if_stmt($3, $5, $7); }
				   ;

iteration_statement: WHILE '(' expression ')' statement 						{ $$ = while_stmt($3, $5); }
			  	   ;

io_statement: READ '(' ID ')' ';'												{ $$ = input($3); }
			| PRINT '(' assignment_expression ')' ';'							{ $$ = output(false, $3); }
			| PRINTLN '(' assignment_expression ')' ';'							{ $$ = output(true, $3); }
			;

expression: assignment_expression												{ $$ = $1; }
		  | expression ',' assignment_expression								{ $$ = expr($1, $3); }
		  ;

assignment_expression: conditional_expression									{ $$ = $1; }
					 | unary_expression '=' assignment_expression				{ $$ = assign($3, $1); }
					 ;

conditional_expression: logical_or_expression									{ $$ = $1; }
					  ;

logical_or_expression: logical_and_expression									{ $$ = $1; }
					 | logical_or_expression OR_OP logical_and_expression		{ $$ = logical_or($1, $3); }
					 ;

logical_and_expression: inclusive_or_expression									{ $$ = $1; }
					  | logical_and_expression AND_OP inclusive_or_expression	{ $$ = logical_and($1, $3); }
					  ;

inclusive_or_expression: exclusive_or_expression								{ $$ = $1; }
					   | inclusive_or_expression '|' exclusive_or_expression	{ $$ = bitwise_or($1, $3); }
					   ;

exclusive_or_expression: and_expression											{ $$ = $1; }
					   | exclusive_or_expression '^' and_expression				{ $$ = bitwise_xor($1, $3); }
					   ;

and_expression: equality_expression												{ $$ = $1; }
			  | and_expression '&' equality_expression							{ $$ = bitwise_and($1, $3); }
			  ;

equality_expression: relational_expression										{ $$ = $1; }
				   | equality_expression EQ_OP relational_expression			{ $$ = equality($1, $3); }
				   | equality_expression NE_OP relational_expression			{ $$ = not_equality($1, $3); }
				   ;

relational_expression: additive_expression										{ $$ = $1; }
					 | relational_expression '<' additive_expression			{ $$ = less_than($1, $3); }
					 | relational_expression '>' additive_expression			{ $$ = greater_than($1, $3); }
					 | relational_expression LE_OP additive_expression			{ $$ = less_than_or_equal($1, $3); }
					 | relational_expression GE_OP additive_expression			{ $$ = greater_than_or_equal($1, $3); }
					 ;

additive_expression: multiplicative_expression									{ $$ = $1; }
				   | additive_expression '+' multiplicative_expression			{ $$ = add($1, $3); }
				   | additive_expression '-' multiplicative_expression			{ $$ = sub($1, $3); }
				   ;

multiplicative_expression: cast_expression										{ $$ = $1; }
						 | multiplicative_expression '*' cast_expression		{ $$ = mul($1, $3); }
						 | multiplicative_expression '/' cast_expression		{ $$ = ast_div($1, $3); }
						 | multiplicative_expression '%' cast_expression		{ $$ = mod($1, $3); }
						 ;

cast_expression: unary_expression												{ $$ = $1; }
			   | '(' type_specifier ')' cast_expression							{ $$ = cast_expr($2, $4); }
			   ;

unary_expression: postfix_expression											{ $$ = $1; }
		  		| unary_operator unary_expression								{ 
																					if ($1 == '-') {
																						$$ = unary_op(AST_UNARY_MIUNS, $2);
																					} else {
																						$$ = $2;
																					}
																				}
				;

unary_operator: '+'																{ $$ = '+'; }
			  | '-'																{ $$ = '-'; }
			  ;

postfix_expression: primary_expression											{ $$ = $1; }
				  ;

primary_expression: ID															{
																					symbol *sym = st_lookup($1);
																					if (sym == NULL) {
																						yyerror(root, "use of undeclared identifier '%s'\n", $1);
																						YYABORT;
																					}
																					$$ = id(sym->ctype, sym->name);
																					free($1);
																				}
				  | NUMBER														{ $$ = integer_literal(ctype_int, $1); }
				  | CHAR_CONST													{ $$ = integer_literal(ctype_char, $1); }
				  | STRING_CONST												{ $$ = string($1); }
				  | BOOL_VAL													{ $$ = integer_literal(ctype_bool, $1); }
				  | '(' expression ')'											{ $$ = $2; }
				  ;
				  
%%

void yyerror(ast_node** root, char *s, ...)
{
    va_list args;

    va_start(args, s);
	fflush(stderr);
    vfprintf(stderr, s, args);
    va_end(args);
}