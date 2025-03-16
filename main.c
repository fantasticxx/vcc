#include "vcc.h"
#include "ast.h"

extern int yydebug;
extern FILE *yyin;

FILE *obj_f;
FILE *err_f;
char *file_name;

static void usage()
{
    fprintf(stdout,
            "vcc [options] filename\n"
            "OPTIONS\n"
            "  -o filename            Write output to the specified file.\n");
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
}

int main(int argc, char *argv[])
{
	processing_cmd_arg(argc, argv);
	start_up_processing();
	st_initialize();
	ast_node *root = NULL;
	#ifdef YYDEBUG
		yydebug = 0;
	#endif
	int err = yyparse(&root);
	if (err == 0) {
		ast_tree_traversal(root);
		codegen(root);
	}
	clean_up_processing();

	return err;
}