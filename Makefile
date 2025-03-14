TARGET = vcc

CC ?= gcc
CFLAGS = -Wall -Werror -std=gnu99 -c -g
LEX := flex
YACC := bison
YFLAGS := -d -t -v -Wother --report=itemset

OUT := bin

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

#$(TARGET): $(OBJS)
#	$(CC) -o $@ $(OBJS)

$(TARGET): main.o lex.yy.o yacc.tab.o ast.o symtab.o codegen.o
	$(CC) -o $@ main.o yacc.tab.o lex.yy.o ast.o symtab.o codegen.o

main.o: main.c yacc.tab.h
	$(CC) -o $@ $< $(CFLAGS)

symtab.o: symtab.c yacc.tab.h
	$(CC) -o $@ $< $(CFLAGS)

ast.o: ast.c
	$(CC) -o $@ $< $(CFLAGS)

codegen.o: codegen.c yacc.tab.h
	$(CC) -o $@ $< $(CFLAGS)

lex.yy.o: lex.yy.c yacc.tab.h
	$(CC) $(CFLAGS) -o $@ $<

yacc.tab.o: yacc.tab.c
	$(CC) $(CFLAGS) -o $@ $<

lex.yy.c: lex.l
	$(LEX) $<

yacc.tab.c yacc.tab.h: yacc.y
	$(YACC) $(YFLAGS) $<

.PHONY: clean
clean:
	$(RM) $(TARGET) lex.yy.c yacc.tab.c yacc.output yacc.tab.h $(OBJS)