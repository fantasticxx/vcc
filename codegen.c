#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "vcc.h"
#include "codegen.h"
#include "symtab.h"
#include "yacc.tab.h"
#include "ast.h"

#define REG_SIZE 6

static const char *REG64[REG_SIZE] = {"rsi", "rdi", "r8", "r9", "r10", "r11"};
static const char *REG32[REG_SIZE] = {"esi", "edi", "r8d", "r9d", "r10d", "r11d"};
static const char *REG16[REG_SIZE] = {"si", "di", "r8w", "r9w", "r10w", "r11w"};
static const char *REG8[REG_SIZE] = {"sil", "dil", "r8b", "r9b", "r10b", "r11b"};
static int registers_count = 0;

static const char *PROG_F = "Vanilla-C compiler generated code";
static const char *VCC_version = "1.0";
static int stack_pos;

static int align(int n, int m)
{
    int rem = n % m;
    return (rem == 0) ? n : n - rem + m;
}

void emit_label(int L)
{
	fprintf(obj_f, "_L%4d:\n", L);
}

static void push(int reg)
{
    fprintf(obj_f, "    push %s\n", REG64[reg]);
}

static void pop(int reg)
{
    fprintf(obj_f, "    pop %s\n", REG64[reg]);
}

static int allocate_register()
{
    if (registers_count < REG_SIZE) {
        return registers_count++;
    }
    push(registers_count % REG_SIZE);
    int reg = registers_count % REG_SIZE;
    registers_count++;
    return reg;
}

static void free_register()
{
    if (registers_count > REG_SIZE) {
        pop((registers_count - 1) % REG_SIZE);
    }
    if (registers_count > 0) {
        registers_count--;
    }
}

static void eval_data_size(int reg, Ctype* parent, Ctype* child)
{
    if (parent->size > child->size) {
        if (parent->size == 8 && child->size == 4) {
            fprintf(obj_f, "    movsxd %s, %s\n", REG64[reg], REG32[reg]);
        } else if (parent->size == 8 && child->size == 1) {
            fprintf(obj_f, "    movsx %s, %s\n", REG64[reg], REG8[reg]);
        } else if (parent->size == 4 && child->size == 1) {
            fprintf(obj_f, "    movsx %s, %s\n", REG32[reg], REG8[reg]);
        }
    }
}

static const char *get_int_reg(int reg, Ctype* ctype)
{
    if (ctype->size == 8) {
        return REG64[reg];
    } else if (ctype->size == 4) {
        return REG32[reg];
    } else if (ctype->size == 2) {
        return REG16[reg];
    } else {
        return REG8[reg];
    }
}

static const char* get_size_directive(Ctype* ctype)
{
    if (ctype->size == 8) {
        return "qword";
    } else if (ctype->size == 4) {
        return "dword";
    } else if (ctype->size == 2) {
        return "word";
    } else {
        return "byte";
    }
}

void emit_comments(const char *comment, ...)
{
	va_list args;
	va_start(args, comment);
	fprintf(obj_f, "; ");
	vfprintf(obj_f, comment, args);
	va_end(args);
}

void emit_title(void)
{
	emit_comments("TITLE: %s\n", PROG_F);
	emit_comments("VCC: %s\n", VCC_version);
    emit_comments("FILE: %s\n", file_name);
	fprintf(obj_f, "\n");
}

void emit_prolouge(ast_node *root)
{
	fprintf(obj_f, "global _%s\n", root->fname);
	fprintf(obj_f, "section .text\n\n");
	fprintf(obj_f, "_%s:\n", root->fname);
	fprintf(obj_f, "    push rbp\n");
	fprintf(obj_f, "    mov rbp, rsp\n");
	int off = 0;
	symbol *s = symtab;
	while (s) {
		off += s->ctype->size;
		s->offset = off;
		s = s->hh.next;
	}
	stack_pos = align(off, 16);
	fprintf(obj_f, "    sub rsp, %d\n", stack_pos);
}

void emit_epilouge(void)
{
	fprintf(obj_f, "    xor eax, eax\n");
	fprintf(obj_f, "    mov rsp, rbp\n");
	fprintf(obj_f, "    pop rbp\n");
	fprintf(obj_f, "    ret\n");
}

static int emit_load(long offset, Ctype* ctype)
{
    int reg = allocate_register();
    fprintf(obj_f, "    mov %s, %s [rbp - %ld]\n", get_int_reg(reg, ctype), get_size_directive(ctype), offset);
    return reg;
}

static void emit_store(int reg, long offset, Ctype* ctype)
{
    fprintf(obj_f, "    mov %s [rbp - %ld], %s\n", get_size_directive(ctype), offset, get_int_reg(reg, ctype));
}

static int emit_int(int val, Ctype* ctype)
{
	int reg = allocate_register();
    fprintf(obj_f, "    mov %s, %d\n", get_int_reg(reg, ctype), val);
    return reg;
}

static int emit_add(int reg1, int reg2, Ctype* ctype)
{
    fprintf(obj_f, "    add %s, %s\n", get_int_reg(reg1, ctype), get_int_reg(reg2, ctype));
    free_register();
    return reg1;
}

static int emit_sub(int reg1, int reg2, Ctype* ctype)
{
    fprintf(obj_f, "    sub %s, %s\n", get_int_reg(reg1, ctype), get_int_reg(reg2, ctype));
    free_register();
    return reg1;
}

static int emit_mul(int reg1, int reg2, Ctype* ctype)
{
    fprintf(obj_f, "    imul %s, %s\n", get_int_reg(reg1, ctype), get_int_reg(reg2, ctype));
    free_register();
    return reg1;
}

static int emit_div(int reg1, int reg2, Ctype* ctype)
{
    if (ctype == ctype_long) {
        fprintf(obj_f, "    mov rax, %s\n", get_int_reg(reg1, ctype));
        fprintf(obj_f, "    mov rdx, 0\n");
        fprintf(obj_f, "    cqo\n");
        fprintf(obj_f, "    idiv %s\n", get_int_reg(reg2, ctype));
        fprintf(obj_f, "    mov %s, rax\n", get_int_reg(reg1, ctype));
    } else if (ctype == ctype_int) {
        fprintf(obj_f, "    mov eax, %s\n", get_int_reg(reg1, ctype));
        fprintf(obj_f, "    mov edx, 0\n");
        fprintf(obj_f, "    cdq\n");
        fprintf(obj_f, "    idiv %s\n", get_int_reg(reg2, ctype));
        fprintf(obj_f, "    mov %s, eax\n", get_int_reg(reg1, ctype));
    }
    free_register();
    return reg1;
}

static int emit_mod(int reg1, int reg2, Ctype* ctype)
{
    if (ctype == ctype_long) {
        fprintf(obj_f, "    mov rax, %s\n", get_int_reg(reg1, ctype));
        fprintf(obj_f, "    mov rdx, 0\n");
        fprintf(obj_f, "    cqo\n");
        fprintf(obj_f, "    idiv %s\n", get_int_reg(reg2, ctype));
        fprintf(obj_f, "    mov %s, rdx\n", get_int_reg(reg1, ctype));
    } else if (ctype == ctype_int) {
        fprintf(obj_f, "    mov eax, %s\n", get_int_reg(reg1, ctype));
        fprintf(obj_f, "    mov edx, 0\n");
        fprintf(obj_f, "    cdq\n");
        fprintf(obj_f, "    idiv %s\n", get_int_reg(reg2, ctype));
        fprintf(obj_f, "    mov %s, edx\n", get_int_reg(reg1, ctype));
    }
    free_register();
    return reg1;
}

static int emit_neg(int reg, Ctype* ctype)
{
    fprintf(obj_f, "    neg %s\n", get_int_reg(reg, ctype));
    return reg;
}

static int emit_lt(int reg1, int reg2, Ctype* ctype)
{
    fprintf(obj_f, "    cmp %s, %s\n", get_int_reg(reg1, ctype), get_int_reg(reg2, ctype));
    fprintf(obj_f, "    setl %s\n", get_int_reg(reg1, ctype_char));
    fprintf(obj_f, "    movzx %s, %s\n", get_int_reg(reg1, ctype_int), get_int_reg(reg1, ctype_char));
    free_register();
    return reg1;
}

static int emit_gt(int reg1, int reg2, Ctype* ctype)
{
    fprintf(obj_f, "    cmp %s, %s\n", get_int_reg(reg1, ctype), get_int_reg(reg2, ctype));
    fprintf(obj_f, "    setg %s\n", get_int_reg(reg1, ctype_char));
    fprintf(obj_f, "    movzx %s, %s\n", get_int_reg(reg1, ctype_int), get_int_reg(reg1, ctype_char));
    free_register();
    return reg1;
}

static int emit_le(int reg1, int reg2, Ctype* ctype)
{
    fprintf(obj_f, "    cmp %s, %s\n", get_int_reg(reg1, ctype), get_int_reg(reg2, ctype));
    fprintf(obj_f, "    setle %s\n", get_int_reg(reg1, ctype_char));
    fprintf(obj_f, "    movzx %s, %s\n", get_int_reg(reg1, ctype_int), get_int_reg(reg1, ctype_char));
    free_register();
    return reg1;
}

static int emit_ge(int reg1, int reg2, Ctype* ctype)
{
    fprintf(obj_f, "    cmp %s, %s\n", get_int_reg(reg1, ctype), get_int_reg(reg2, ctype));
    fprintf(obj_f, "    setge %s\n", get_int_reg(reg1, ctype_char));
    fprintf(obj_f, "    movzx %s, %s\n", get_int_reg(reg1, ctype_int), get_int_reg(reg1, ctype_char));
    free_register();
    return reg1;
}

static int emit_eq(int reg1, int reg2, Ctype* ctype)
{
    fprintf(obj_f, "    cmp %s, %s\n", get_int_reg(reg1, ctype), get_int_reg(reg2, ctype));
    fprintf(obj_f, "    sete %s\n", get_int_reg(reg1, ctype_char));
    fprintf(obj_f, "    movzx %s, %s\n", get_int_reg(reg1, ctype_int), get_int_reg(reg1, ctype_char));
    free_register();
    return reg1;
}

static int emit_ne(int reg1, int reg2, Ctype* ctype)
{
    fprintf(obj_f, "    cmp %s, %s\n", get_int_reg(reg1, ctype), get_int_reg(reg2, ctype));
    fprintf(obj_f, "    setne %s\n", get_int_reg(reg1, ctype_char));
    fprintf(obj_f, "    movzx %s, %s\n", get_int_reg(reg1, ctype_int), get_int_reg(reg1, ctype_char));
    free_register();
    return reg1;
}

static int emit_and(int reg1, int reg2, Ctype* ctype)
{
    fprintf(obj_f, "    and %s, %s\n", get_int_reg(reg1, ctype), get_int_reg(reg2, ctype));
    free_register();
    return reg1;
}

static int emit_or(int reg1, int reg2, Ctype* ctype)
{
    fprintf(obj_f, "    or %s, %s\n", get_int_reg(reg1, ctype), get_int_reg(reg2, ctype));
    free_register();
    return reg1;
}

static int emit_xor(int reg1, int reg2, Ctype* ctype)
{
    fprintf(obj_f, "    xor %s, %s\n", get_int_reg(reg1, ctype), get_int_reg(reg2, ctype));
    free_register();
    return reg1;
}

// static void emit_logic_and(int reg1, int reg2, Ctype* ctype)
// {

// }

static int emit_code(ast_node *root)
{
	if (!root) {
        return 0;
    }
    int reg;
    symbol *s = NULL;

	switch (root->type) {
	case AST_LITERAL:
		return emit_int(root->ival, root->ctype);
	case AST_ID:
		s = st_lookup(root->varname);
        if (s == NULL) {
            fprintf(stderr, "codegen: unknown variable %s", root->varname);
            exit(1);
        }
		return emit_load(s->offset, root->ctype);
    case AST_UNARY_MIUNS:
        reg = emit_code(root->operand);
        return emit_neg(reg, root->ctype);
    case AST_BLOCK_ITEM_LIST:
        emit_code(root->left);
        emit_code(root->right);
        return -1;
    case AST_DECL:
        emit_code(root->init_list);
        return -1;
    case AST_INIT_DECL_LIST:
        emit_code(root->left);
        emit_code(root->right);
        return -1;
    case AST_INIT_DECL:
        reg = emit_code(root->left);
        s = st_lookup(root->right->varname);
        if (s == NULL) {
            fprintf(stderr, "codegen: unknown variable %s", root->right->varname);
            exit(1);
        }
        eval_data_size(reg, s->ctype, root->left->ctype);
        emit_store(reg, s->offset, root->ctype);
        free_register();
        return -1;
    case AST_ASSIGN:
        reg = emit_code(root->left);
        s = st_lookup(root->right->varname);
        if (s == NULL) {
            fprintf(stderr, "codegen: unknown variable %s", root->right->varname);
            exit(1);
        }
        eval_data_size(reg, s->ctype, root->left->ctype);
        emit_store(reg, s->offset, root->ctype);
        return reg;
    case AST_DIRECT_DECL:
        return -1;
	default:
		break;
	}

	int reg1 = emit_code(root->left);
    int reg2 = emit_code(root->right);

    eval_data_size(reg1, root->left->ctype, root->right->ctype);
    eval_data_size(reg2, root->left->ctype, root->right->ctype);

    switch (root->type) {
    case AST_ADD:
        return emit_add(reg1, reg2, root->ctype);
    case AST_SUB:
        return emit_sub(reg1, reg2, root->ctype);
    case AST_MUL:
        return emit_mul(reg1, reg2, root->ctype);
    case AST_DIV:
        return emit_div(reg1, reg2, root->ctype);
    case AST_MOD:
        return emit_mod(reg1, reg2, root->ctype);
    case AST_LT:
        return emit_lt(reg1, reg2, root->ctype);
    case AST_GT:
        return emit_gt(reg1, reg2, root->ctype);
    case AST_LE:
        return emit_le(reg1, reg2, root->ctype);
    case AST_GE:
        return emit_ge(reg1, reg2, root->ctype);
    case AST_EQ:
        return emit_eq(reg1, reg2, root->ctype);
    case AST_NE:
        return emit_ne(reg1, reg2, root->ctype);
    case AST_BITWISE_AND:
        return emit_and(reg1, reg2, root->ctype);
    case AST_BITWISE_OR:
        return emit_or(reg1, reg2, root->ctype);
    case AST_BITWISE_XOR:
        return emit_xor(reg1, reg2, root->ctype);
    default:
        fprintf(stderr, "codegen: unknown AST node type");
        exit(1);
    }
}

void codegen(ast_node* root)
{
	emit_title();
	stack_pos = 0;
	emit_prolouge(root);
	emit_code(root->body);
	emit_epilouge();
}