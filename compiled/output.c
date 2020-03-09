/*
  Memory map:

  SP
  AP
  Hash
  Stack

  SP,AP作戦は複雑な関数の部分適用で壊れないことを確認するべき.

  hashの必要な大きさは予想できそう
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define uint unsigned int
#define PUSHF(x) AP->fp = x; AP++; SP = AP;
#define PUSHV(x) AP->val = x; AP++; SP = AP;
#define PUSHS(x) AP->str = x; AP++; SP = AP;
#define PUSHP(x) AP->ptr = x; AP++; SP = AP;
#define STACK(x) (AP-(x))
#define TOP() (SP-1)
#define MPOP() AP--; SP = AP;
#define POP() SP--;



#define mallocate malloc


 // 今はstackをvoid*固定としましょうか

// 番兵を使わないタイプのメモリ
typedef struct {
    void *ptr;
    unsigned int size;
    unsigned int sizeexpo;
} ptr_t;

union memory_t {
    void(*fp)();
    int val;
    // 番兵を使うタイプのメモリ（文字列）
    char *str;
    // 番兵を使わないタイプのメモリ
    ptr_t *ptr;
};

union memory_t *SP, *AP, ACC;
void(*HP[256])();
union memory_t *MEM;

ptr_t *ALLOCATE(unsigned int size) {
    ptr_t *res = mallocate(sizeof(ptr_t));
    res->ptr = mallocate(size);
    res->size = size;
    res->sizeexpo = 3;
    return res;
}

void FREE(ptr_t *ptr) {
    free(ptr->ptr);
    free(ptr);
}


void negate() {
    int a = TOP()->val; MPOP();
    PUSHV(-a);
    return;
}
void sub() {
    int a = TOP()->val; MPOP();
    int b = TOP()->val; MPOP();
    PUSHV(a - b);
    return;
}
void add() {
//    (SP+(-1))->fp();
//    (SP+(-3))->fp();
    
    int a = TOP()->val; MPOP();
    int b = TOP()->val; MPOP();

//    MPOP();MPOP(); // 関数の方も削除
    
    PUSHV(a + b);
    return;
}
void mul() {
    int a = TOP()->val; MPOP();
    int b = TOP()->val; MPOP();

    PUSHV(a * b);
    return;
}

void concat() {
//    (SP+(-1))->fp();
//    (SP+(-3))->fp();

    char *a = TOP()->str; MPOP();
    char *b = TOP()->str; MPOP();

    // TODO: ここのメモリを開放する手段がないのでまだ遣っちゃダメ
    ptr_t *new = ALLOCATE(strlen(a) + strlen(b) + 1);

    strcpy(new->ptr, a);
    strcat(new->ptr, b);
//    MPOP();MPOP(); // 関数の方も削除

    PUSHP(new);
    return;
}


#include "env.c"
// #include "env.c.new"


int main() {
stack_init:
    MEM = calloc(256, sizeof(union memory_t));
    AP = SP = MEM;
    // HP[0] = ADD;
    // HP[1] = SUB;
    // HP[2] = MUL;
    // HP[3] = NEG;

start:
#include "code.c"

    printf("%d\n", SP-MEM);
    printf("%d\n", STACK(1)->val);
    
exit:
    free(MEM);

funcs:

    return 0;
}

