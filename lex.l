%{
#include "vcc.h"

// TODO: Handle underscores '_' in identifiers 
%}

%option yylineno
%option noyywrap
%option nounput
%option noinput

%x COMMENT CPP_COMMENT

DIGIT					          [[:digit:]]
LETTER					        [[:alpha:]]
ID 						          {LETTER}({LETTER}|{DIGIT})*
NUMBER					        {DIGIT}+
EOL						          [\n|\r|\r\n]
WHITE_SPACE				      [ \t\v\f]|{EOL}
CHAR_CONST				      \'([^\'\\]|\\.)*\'
STRING_CONST			      \"([^\"\\]|\\.)*\"

%%
"True"                  { yylval.ival = 1; return BOOL_VAL; }
"False"	                { yylval.ival = 0; return BOOL_VAL; }
"bool"					        { return BOOL;}
"char"					        { return CHAR;}
"const"					        { return CONST;}
"else"					        { return ELSE;}
"if"					          { return IF;}
"int"					          { return INT;}
"main"					        { yylval.sval = strdup(yytext); return MAIN; }
"print"					        { return PRINT;}
"println"				        { return PRINTLN;}
"read"					        { return READ;}
"string"				        { return STRING;}
"while"					        { return WHILE;}

{ID}					          { /* Identifier must be parsed after reserved words */
                          yylval.sval = strdup(yytext); return ID;
                        }

"+"						          { return '+'; }
"-"						          { return '-'; }
"*"						          { return '*'; }
"/"						          { return '/'; }
"%"						          { return '%'; }
"("						          { return '('; }
")"						          { return ')'; }
"{"						          { return '{'; }
"}"						          { return '}'; }
"="						          { return '='; }
","						          { return ','; }
";"						          { return ';'; }
"<"						          { return '<'; }
">"						          { return '>'; }
"|"                     { return '|'; }
"^"                     { return '^'; }
"&"                     { return '&'; }
"&&"                    { return AND_OP; }
"||"                    { return OR_OP; }
"<="					          { return LE_OP; }
">="					          { return GE_OP; }
"=="					          { return EQ_OP; }
"!="					          { return NE_OP; }

{NUMBER}				        { yylval.ival = atoi(yytext); return NUMBER;}
{CHAR_CONST}			      { yylval.cval = yytext[1]; return CHAR_CONST;}
{STRING_CONST}			    { yylval.sval = strdup(yytext); return STRING_CONST;}
{WHITE_SPACE}			      { }

.						            { yyerror(NULL, "%d: Unexpected character: %c", yylineno, yytext[0]);}

<INITIAL>"/*"           { BEGIN(COMMENT); }
<COMMENT>"*/"           { BEGIN(INITIAL); }
<COMMENT>.|{EOL}  		  { /* Ignore characters inside comments */ }
<COMMENT><<EOF>>        {
                            yyerror(NULL, "\n%d: Unterminated comment\n", yylineno);
                            yyterminate();
                        }

<INITIAL>"//"           { BEGIN(CPP_COMMENT); }
<CPP_COMMENT>{EOL}      { BEGIN(INITIAL);/*return '\n';*/}
<CPP_COMMENT>.          { /* Ignore characters inside C++-style comments */ }
%%