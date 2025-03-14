#include "vcc.h"
#include "reg.h"

#define REG_SIZE 6

const char *REG64[REG_SIZE] = {"rsi", "rdi", "r8", "r9", "r10", "r11"};
const char *REG32[REG_SIZE] = {"esi", "edi", "r8d", "r9d", "r10d", "r11d"};
const char *REG16[REG_SIZE] = {"si", "di", "r8w", "r9w", "r10w", "r11w"};
const char *REG8[REG_SIZE] = {"sil", "dil", "r8b", "r9b", "r10b", "r11b"};

int reg_count = 0;

const char* reg_allocate()
{

}

void reg_free()
{

}

void reg_bookkeeping()
{

}

void reg_clear_book()
{

}

void reg_push_book()
{

}

void reg_pop_book()
{
	
}