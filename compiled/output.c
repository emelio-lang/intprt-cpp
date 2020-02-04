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

#define uint unsigned int
#define PUSHF(x) AP->fp = x; AP++; SP = AP;
#define PUSHV(x) AP->val = x; AP++; SP = AP;
#define STACK(x) (AP-(x))
#define TOP() (SP-1)
#define MPOP() AP--; SP = AP;
#define POP() SP--;

 // 今はstackをvoid*固定としましょうか

union memory_t {
    void(*fp)();
    int val;
};

union memory_t *SP, *AP, ACC;
void(*HP[256])();
union memory_t *MEM;

void NEG() {
    int a = TOP()->val; MPOP();
    PUSHV(-a);
    return;
}
void SUB() {
    int b = TOP()->val; MPOP();
    int a = TOP()->val; MPOP();
    PUSHV(a - b);
    return;
}
void ADD() {
    int b = TOP()->val; MPOP();
    int a = TOP()->val; MPOP();
    
    PUSHV(a + b);
    return;
}
void MUL() {
    printf("a\n");

    int b = TOP()->val; MPOP();
    int a = TOP()->val; MPOP();
    
    PUSHV(a * b);
    return;
}


#include "env.c.new"


int main() {
stack_init:
    MEM = calloc(256, sizeof(union memory_t));
    AP = SP = MEM;
    HP[0] = ADD;
    HP[1] = SUB;
    HP[2] = MUL;
    HP[3] = NEG;

start:
#include "code.c"

    printf("%d\n", SP-MEM);
    printf("%d\n", STACK(1)->val);
    
exit:
    free(MEM);

funcs:

    return 0;
}

