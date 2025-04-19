#include "ast.h"
#include "semantic.h"
#include "symtab.h"
#include "vcc.h"

extern int yydebug;
extern FILE *yyin;

FILE *obj_f;
FILE *err_f;
char *file_name;
int error_count = 0;

static void usage()
{
    fprintf(stderr, "vcc: error: no input files\n./vcc filename\n");
}

static void print_usage_and_exit()
{
    usage();
    exit(1);
}

static void processing_cmd_arg(int argc, char **argv)
{
    if (argc < 2) {
        print_usage_and_exit();
    }
    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL) {
        fprintf(stderr, "File %s not found\n", argv[1]);
        exit(1);
    }
    yyin = fp;
    file_name = argv[1];
}

static void start_up_processing(void)
{
    obj_f = fopen("a.asm", "w+");
}

static void clean_up_processing(void)
{
    fclose(obj_f);
    fclose(yyin);
}

int main(int argc, char *argv[])
{
    processing_cmd_arg(argc, argv);
    st_initialize();
    ast_initialize();
    ast_node *root = NULL;
#ifdef YYDEBUG
    yydebug = 0;
#endif
    int err = yyparse(&root);
    if (err == 0 && error_count == 0) {
        smantic_analysis(root);
        // print_ast(root, 0);
        // print_symbol_table(symtab);
        start_up_processing();
        codegen(root);
        clean_up_processing();
    }
    return err || error_count;
}