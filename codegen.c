#include "codegen.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "symtab.h"
#include "vcc.h"
#include "yacc.tab.h"

#define REG_SIZE 6

static int emit_code(ast_node *root);

static const char *REG64[REG_SIZE] = {"rsi", "rdi", "r8", "r9", "r10", "r11"};
static const char *REG32[REG_SIZE] = {"esi", "edi", "r8d", "r9d", "r10d", "r11d"};
static const char *REG16[REG_SIZE] = {"si", "di", "r8w", "r9w", "r10w", "r11w"};
static const char *REG8[REG_SIZE] = {"sil", "dil", "r8b", "r9b", "r10b", "r11b"};
static int registers_count;
static int label_count;
static int stack_pos;

static void init()
{
    registers_count = 0;
    label_count = 0;
    stack_pos = 0;
}

static int align(int n, int m)
{
    int rem = n % m;
    return (rem == 0) ? n : n - rem + m;
}

static void push(int reg)
{
    stack_pos += 8;
    fprintf(obj_f, "    push %s\n", REG64[reg]);
}

static void pop(int reg)
{
    stack_pos -= 8;
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

static void eval_data_size(int reg, Ctype *parent, Ctype *child)
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

static const char *get_int_reg(int reg, Ctype *ctype)
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

static const char *get_size_directive(Ctype *ctype)
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

static int make_label(void)
{
    return label_count++;
}

static void emit_comments(const char *comment, ...)
{
    va_list args;
    va_start(args, comment);
    fprintf(obj_f, "; ");
    vfprintf(obj_f, comment, args);
    va_end(args);
}

static void emit_title(void)
{
    emit_comments("TITLE: %s\n", PROG_F);
    emit_comments("VCC: %s\n", VCC_version);
    emit_comments("FILE: %s\n", file_name);
    fprintf(obj_f, "\n");
}

static void emit_string_label(int L)
{
    fprintf(obj_f, "_L.str.%d:", L);
}

static void emit_data()
{
    fprintf(obj_f, "section .data\n");
    fprintf(obj_f, "_fmtd: db \"%%d\", 0\n");
    fprintf(obj_f, "_fmtpc: db \"%%c\", 0\n");
    fprintf(obj_f, "_fmtrc: db \" %%c\", 0\n");
    fprintf(obj_f, "_fmts: db \"%%s\", 0\n");
    fprintf(obj_f, "_lf: db 10, 0\n");
    list_reverse(strings);
    Iter iter = list_iter(strings);
    for (int i = 0; i < list_len(strings); i++) {
        ast_node *v = (ast_node *) iter_next(&iter);
        emit_string_label(v->slabel);
        fprintf(obj_f, " db %s, 0\n", v->sval);
    }
    fprintf(obj_f, "\n");
}

static void emit_reserved_label(int L)
{
    fprintf(obj_f, "_L.res.%d:", L);
}

static void emit_bss()
{
    fprintf(obj_f, "section .bss\n");
    list_reverse(reserved);
    Iter iter = list_iter(reserved);
    for (int i = 0; i < list_len(reserved); i++) {
        ast_node *v = (ast_node *) iter_next(&iter);
        emit_reserved_label(v->reserved_label);
        fprintf(obj_f, "  resb %d\n", STRING_RESERVED_BYTES);
    }
    fprintf(obj_f, "\n");
}

static void emit_prolouge(ast_node *root)
{
    fprintf(obj_f, "section .text\n");
    fprintf(obj_f, "global _%s\n", root->fname);
    fprintf(obj_f, "extern _printf\n");
    fprintf(obj_f, "extern _scanf\n\n");
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

static void emit_epilouge(void)
{
    fprintf(obj_f, "    xor eax, eax\n");
    fprintf(obj_f, "    mov rsp, rbp\n");
    fprintf(obj_f, "    pop rbp\n");
    fprintf(obj_f, "    ret");
}

void emit_label(int L)
{
    fprintf(obj_f, "_L%d:\n", L);
}

static int emit_load(int offset, Ctype *ctype)
{
    int dst = allocate_register();
    fprintf(obj_f, "    mov %s, %s [rbp - %d]\n", get_int_reg(dst, ctype),
            get_size_directive(ctype), offset);
    return dst;
}

static void emit_store(int src, int offset, Ctype *ctype)
{
    fprintf(obj_f, "    mov %s [rbp - %d], %s\n", get_size_directive(ctype), offset,
            get_int_reg(src, ctype));
}

static int emit_int(int val, Ctype *ctype)
{
    int dst = allocate_register();
    fprintf(obj_f, "    mov %s, %d\n", get_int_reg(dst, ctype), val);
    return dst;
}

static int emit_string(int slabel)
{
    int dst = allocate_register();
    fprintf(obj_f, "    lea %s, [rel _L.str.%d]\n", get_int_reg(dst, ctype_string), slabel);
    return dst;
}

static int emit_add(int dst, int src, Ctype *ctype)
{
    fprintf(obj_f, "    add %s, %s\n", get_int_reg(dst, ctype), get_int_reg(src, ctype));
    free_register();
    return dst;
}

static int emit_sub(int dst, int src, Ctype *ctype)
{
    fprintf(obj_f, "    sub %s, %s\n", get_int_reg(dst, ctype), get_int_reg(src, ctype));
    free_register();
    return dst;
}

static int emit_mul(int dst, int src, Ctype *ctype)
{
    fprintf(obj_f, "    imul %s, %s\n", get_int_reg(dst, ctype), get_int_reg(src, ctype));
    free_register();
    return dst;
}

static int emit_div(int dividend, int divisor, Ctype *ctype)
{
    if (ctype == ctype_long) {
        fprintf(obj_f, "    mov rax, %s\n", get_int_reg(dividend, ctype));
        fprintf(obj_f, "    mov rdx, 0\n");
        fprintf(obj_f, "    cqo\n");
        fprintf(obj_f, "    idiv %s\n", get_int_reg(divisor, ctype));
        fprintf(obj_f, "    mov %s, rax\n", get_int_reg(dividend, ctype));
    } else if (ctype == ctype_int) {
        fprintf(obj_f, "    mov eax, %s\n", get_int_reg(dividend, ctype));
        fprintf(obj_f, "    mov edx, 0\n");
        fprintf(obj_f, "    cdq\n");
        fprintf(obj_f, "    idiv %s\n", get_int_reg(divisor, ctype));
        fprintf(obj_f, "    mov %s, eax\n", get_int_reg(dividend, ctype));
    }
    free_register();
    return dividend;
}

static int emit_mod(int dividend, int divisor, Ctype *ctype)
{
    if (ctype == ctype_long) {
        fprintf(obj_f, "    mov rax, %s\n", get_int_reg(dividend, ctype));
        fprintf(obj_f, "    mov rdx, 0\n");
        fprintf(obj_f, "    cqo\n");
        fprintf(obj_f, "    idiv %s\n", get_int_reg(divisor, ctype));
        fprintf(obj_f, "    mov %s, rdx\n", get_int_reg(dividend, ctype));
    } else if (ctype == ctype_int) {
        fprintf(obj_f, "    mov eax, %s\n", get_int_reg(dividend, ctype));
        fprintf(obj_f, "    mov edx, 0\n");
        fprintf(obj_f, "    cdq\n");
        fprintf(obj_f, "    idiv %s\n", get_int_reg(divisor, ctype));
        fprintf(obj_f, "    mov %s, edx\n", get_int_reg(dividend, ctype));
    }
    free_register();
    return dividend;
}

static int emit_neg(int dst, Ctype *ctype)
{
    fprintf(obj_f, "    neg %s\n", get_int_reg(dst, ctype));
    return dst;
}

static int emit_lt(int dst, int src, Ctype *ctype)
{
    fprintf(obj_f, "    cmp %s, %s\n", get_int_reg(dst, ctype), get_int_reg(src, ctype));
    fprintf(obj_f, "    setl %s\n", get_int_reg(dst, ctype_char));
    fprintf(obj_f, "    movzx %s, %s\n", get_int_reg(dst, ctype_int), get_int_reg(dst, ctype_char));
    free_register();
    return dst;
}

static int emit_gt(int dst, int src, Ctype *ctype)
{
    fprintf(obj_f, "    cmp %s, %s\n", get_int_reg(dst, ctype), get_int_reg(src, ctype));
    fprintf(obj_f, "    setg %s\n", get_int_reg(dst, ctype_char));
    fprintf(obj_f, "    movzx %s, %s\n", get_int_reg(dst, ctype_int), get_int_reg(dst, ctype_char));
    free_register();
    return dst;
}

static int emit_le(int dst, int src, Ctype *ctype)
{
    fprintf(obj_f, "    cmp %s, %s\n", get_int_reg(dst, ctype), get_int_reg(src, ctype));
    fprintf(obj_f, "    setle %s\n", get_int_reg(dst, ctype_char));
    fprintf(obj_f, "    movzx %s, %s\n", get_int_reg(dst, ctype_int), get_int_reg(dst, ctype_char));
    free_register();
    return dst;
}

static int emit_ge(int dst, int src, Ctype *ctype)
{
    fprintf(obj_f, "    cmp %s, %s\n", get_int_reg(dst, ctype), get_int_reg(src, ctype));
    fprintf(obj_f, "    setge %s\n", get_int_reg(dst, ctype_char));
    fprintf(obj_f, "    movzx %s, %s\n", get_int_reg(dst, ctype_int), get_int_reg(dst, ctype_char));
    free_register();
    return dst;
}

static int emit_eq(int dst, int src, Ctype *ctype)
{
    fprintf(obj_f, "    cmp %s, %s\n", get_int_reg(dst, ctype), get_int_reg(src, ctype));
    fprintf(obj_f, "    sete %s\n", get_int_reg(dst, ctype_char));
    fprintf(obj_f, "    movzx %s, %s\n", get_int_reg(dst, ctype_int), get_int_reg(dst, ctype_char));
    free_register();
    return dst;
}

static int emit_ne(int dst, int src, Ctype *ctype)
{
    fprintf(obj_f, "    cmp %s, %s\n", get_int_reg(dst, ctype), get_int_reg(src, ctype));
    fprintf(obj_f, "    setne %s\n", get_int_reg(dst, ctype_char));
    fprintf(obj_f, "    movzx %s, %s\n", get_int_reg(dst, ctype_int), get_int_reg(dst, ctype_char));
    free_register();
    return dst;
}

static int emit_and(int dst, int src, Ctype *ctype)
{
    fprintf(obj_f, "    and %s, %s\n", get_int_reg(dst, ctype), get_int_reg(src, ctype));
    free_register();
    return dst;
}

static int emit_or(int dst, int src, Ctype *ctype)
{
    fprintf(obj_f, "    or %s, %s\n", get_int_reg(dst, ctype), get_int_reg(src, ctype));
    free_register();
    return dst;
}

static int emit_xor(int dst, int src, Ctype *ctype)
{
    fprintf(obj_f, "    xor %s, %s\n", get_int_reg(dst, ctype), get_int_reg(src, ctype));
    free_register();
    return dst;
}

static int emit_logical_and(List *list)
{
    int reg = allocate_register();
    int end_label = make_label();
    Iter iter = list_iter(list);

    const char *reg_name = get_int_reg(reg, ctype_int);
    fprintf(obj_f, "    xor %s, %s\n", reg_name, reg_name);

    for (int i = 0; i < list_len(list); i++) {
        ast_node *node = (ast_node *) iter_next(&iter);
        int r = emit_code(node);
        const char *s = get_int_reg(r, node->ctype);
        fprintf(obj_f, "    test %s, %s\n", s, s);
        fprintf(obj_f, "    jz _L%d\n", end_label);
        free_register();
    }
    fprintf(obj_f, "    mov %s, 1\n", reg_name);
    emit_label(end_label);
    return reg;
}

static int emit_logical_or(List *list)
{
    int reg = allocate_register();
    int end_label = make_label();
    Iter iter = list_iter(list);

    const char *reg_name = get_int_reg(reg, ctype_int);
    fprintf(obj_f, "    xor %s, %s\n", reg_name, reg_name);

    for (int i = 0; i < list_len(list); i++) {
        ast_node *node = (ast_node *) iter_next(&iter);
        int r = emit_code(node);
        const char *s = get_int_reg(r, node->ctype);
        fprintf(obj_f, "    test %s, %s\n", s, s);
        fprintf(obj_f, "    mov %s, 1\n", reg_name);
        fprintf(obj_f, "    jnz _L%d\n", end_label);
        free_register();
    }
    fprintf(obj_f, "    mov %s, 0\n", reg_name);
    emit_label(end_label);
    return reg;
}

static void emit_if_stmt(ast_node *root)
{
    int reg = emit_code(root->cond);
    int end_label = make_label();
    const char *reg_name = get_int_reg(reg, root->cond->ctype);
    fprintf(obj_f, "    test %s, %s\n", reg_name, reg_name);
    fprintf(obj_f, "    jz _L%d\n", end_label);
    free_register();
    emit_code(root->then);
    free_register();
    if (root->els) {
        int els_label = make_label();
        fprintf(obj_f, "    jmp _L%d\n", els_label);
        emit_label(end_label);
        emit_code(root->els);
        emit_label(els_label);
        free_register();
    } else {
        emit_label(end_label);
    }
}

static void emit_while_stmt(ast_node *root)
{
    int begin_label = make_label();
    int end_label = make_label();
    emit_label(begin_label);
    int reg = emit_code(root->whilecond);
    const char *reg_name = get_int_reg(reg, root->whilecond->ctype);
    fprintf(obj_f, "    test %s, %s\n", reg_name, reg_name);
    fprintf(obj_f, "    jz _L%d\n", end_label);
    free_register();
    emit_code(root->whilebody);
    fprintf(obj_f, "    jmp _L%d\n", begin_label);
    emit_label(end_label);
}

static void emit_print(int reg, Ctype *ctype, bool lf)
{
    push(0);
    push(1);
    int align = stack_pos % 16;
    if (align != 0) {
        fprintf(obj_f, "    sub rsp, %d\n", align);
        stack_pos += align;
    }
    if (ctype == ctype_char) {
        fprintf(obj_f, "    movsx %s, %s\n", REG64[0], get_int_reg(reg, ctype));
        fprintf(obj_f, "    lea rdi, [rel _fmtpc]\n");
    } else if (ctype == ctype_bool) {
        fprintf(obj_f, "    movsx %s, %s\n", REG64[0], get_int_reg(reg, ctype));
        fprintf(obj_f, "    lea rdi, [rel _fmtpc]\n");
    } else if (ctype == ctype_int) {
        fprintf(obj_f, "    movsxd %s, %s\n", REG64[0], get_int_reg(reg, ctype));
        fprintf(obj_f, "    lea rdi, [rel _fmtd]\n");
    } else {
        fprintf(obj_f, "    mov %s, %s\n", REG64[1], get_int_reg(reg, ctype_string));
    }
    fprintf(obj_f, "    xor rax, rax\n");
    fprintf(obj_f, "    call _printf\n");
    if (lf) {
        fprintf(obj_f, "    lea rdi, [rel _lf]\n");
        fprintf(obj_f, "    xor rax, rax\n");
        fprintf(obj_f, "    call _printf\n");
    }
    if (align != 0) {
        fprintf(obj_f, "    add rsp, %d\n", align);
        stack_pos -= align;
    }
    pop(1);
    pop(0);
    free_register();
}

static void emit_read(Ctype *ctype, int offset)
{
    push(0);
    push(1);
    int align = stack_pos % 16;
    if (align != 0) {
        fprintf(obj_f, "    sub rsp, %d\n", align);
        stack_pos += align;
    }
    if (ctype == ctype_char || ctype == ctype_bool) {
        fprintf(obj_f, "    lea rdi, [rel _fmtrc]\n");
        fprintf(obj_f, "    lea rsi, [rbp - %d]\n", offset);
    } else if (ctype == ctype_int) {
        fprintf(obj_f, "    lea rdi, [rel _fmtd]\n");
        fprintf(obj_f, "    lea rsi, [rbp - %d]\n", offset);
    } else {
        fprintf(obj_f, "    lea rdi, [rel _fmts]\n");
        fprintf(obj_f, "    mov rsi, qword [rbp - %d]\n", offset);
    }
    fprintf(obj_f, "    xor rax, rax\n");
    fprintf(obj_f, "    call _scanf\n");
    if (align != 0) {
        fprintf(obj_f, "    add rsp, %d\n", align);
        stack_pos -= align;
    }
    pop(1);
    pop(0);
}

static void emit_reseved(int offset, int reserved_label)
{
    int dst = allocate_register();
    fprintf(obj_f, "    lea %s, [rel _L.res.%d]\n", get_int_reg(dst, ctype_string), reserved_label);
    emit_store(dst, offset, ctype_string);
    free_register();
}

static int emit_code(ast_node *root)
{
    if (!root) {
        return -1;
    }
    int reg;
    symbol *s = NULL;

    switch (root->type) {
    case AST_LITERAL:
        return emit_int(root->ival, root->ctype);
    case AST_STRING:
        return emit_string(root->slabel);
    case AST_ID:
        s = st_lookup(root->varname);
        if (s == NULL) {
            fprintf(stderr, "codegen: unknown variable %s", root->varname);
            exit(1);
        }
        return emit_load(s->offset, s->ctype);
    case AST_UNARY_MIUNS:
        reg = emit_code(root->operand);
        return emit_neg(reg, root->ctype);
    case AST_BLOCK_ITEM_LIST:
        emit_code(root->left);
        free_register();
        emit_code(root->right);
        free_register();
        return -1;
    case AST_DECL:
        emit_code(root->init_list);
        return -1;
    case AST_INIT_DECL_LIST:
        emit_code(root->left);
        emit_code(root->right);
        return -1;
    case AST_INIT_DECL:
        reg = emit_code(root->declinit);
        s = st_lookup(root->declvar->varname);
        if (s == NULL) {
            fprintf(stderr, "codegen: unknown variable %s", root->declvar->varname);
            exit(1);
        }
        eval_data_size(reg, root->ctype, root->declinit->ctype);
        emit_store(reg, s->offset, s->ctype);
        free_register();
        return -1;
    case AST_ASSIGN:
        reg = emit_code(root->left);
        s = st_lookup(root->right->varname);
        if (s == NULL) {
            fprintf(stderr, "codegen: unknown variable %s", root->right->varname);
            exit(1);
        }
        eval_data_size(reg, root->ctype, root->left->ctype);
        emit_store(reg, s->offset, root->ctype);
        return reg;
    case AST_DIRECT_DECL:
        if (root->ctype == ctype_string && root->reserved_label >= 0) {
            s = st_lookup(root->varname);
            emit_reseved(s->offset, root->reserved_label);
        }
        return -1;
    case AST_LOGICAL_AND:
        return emit_logical_and(root->head);
    case AST_LOGICAL_OR:
        return emit_logical_or(root->head);
    case AST_EXPR:
        emit_code(root->left);
        free_register();
        return emit_code(root->right);
    case AST_CAST_EXPR:
        reg = emit_code(root->operand);
        eval_data_size(reg, root->ctype, root->operand->ctype);
        return reg;
    case AST_IF:
        emit_if_stmt(root);
        return -1;
    case AST_WHILE:
        emit_while_stmt(root);
        return -1;
    case AST_OUTPUT:
        reg = emit_code(root->assign_expr);
        emit_print(reg, root->assign_expr->ctype, root->lf);
        return -1;
    case AST_INPUT:
        s = st_lookup(root->varname);
        emit_read(s->ctype, s->offset);
        return -1;
    default:
        break;
    }

    int dst = emit_code(root->left);
    int src = emit_code(root->right);

    eval_data_size(dst, root->left->ctype, root->right->ctype);
    eval_data_size(src, root->left->ctype, root->right->ctype);

    switch (root->type) {
    case AST_ADD:
        return emit_add(dst, src, root->ctype);
    case AST_SUB:
        return emit_sub(dst, src, root->ctype);
    case AST_MUL:
        return emit_mul(dst, src, root->ctype);
    case AST_DIV:
        return emit_div(dst, src, root->ctype);
    case AST_MOD:
        return emit_mod(dst, src, root->ctype);
    case AST_LT:
        return emit_lt(dst, src, root->ctype);
    case AST_GT:
        return emit_gt(dst, src, root->ctype);
    case AST_LE:
        return emit_le(dst, src, root->ctype);
    case AST_GE:
        return emit_ge(dst, src, root->ctype);
    case AST_EQ:
        return emit_eq(dst, src, root->ctype);
    case AST_NE:
        return emit_ne(dst, src, root->ctype);
    case AST_BITWISE_AND:
        return emit_and(dst, src, root->ctype);
    case AST_BITWISE_OR:
        return emit_or(dst, src, root->ctype);
    case AST_BITWISE_XOR:
        return emit_xor(dst, src, root->ctype);
    default:
        fprintf(stderr, "codegen: unknown AST node type");
        exit(1);
    }
}

void codegen(ast_node *root)
{
    init();
    emit_title();
    emit_data();
    emit_bss();
    emit_prolouge(root);
    emit_code(root->body);
    emit_epilouge();
}