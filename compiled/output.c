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

void negate() {
    int a = TOP()->val; MPOP();
    PUSHV(-a);
    return;
}
void sub() {
    int b = TOP()->val; MPOP();
    int a = TOP()->val; MPOP();
    PUSHV(a - b);
    return;
}
void add() {
//    (SP+(-1))->fp();
//    (SP+(-3))->fp();
    
    int b = TOP()->val; MPOP();
    int a = TOP()->val; MPOP();

//    MPOP();MPOP(); // 関数の方も削除
    
    PUSHV(a + b);
    return;
}
void mul() {
    int b = TOP()->val; MPOP();
    int a = TOP()->val; MPOP();

    PUSHV(a * b);
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

