#include <stdio.h>
int g = 7;

int foo(int a, int b) {
    return a + g;
}

int main() {
    printf("%d\n", foo(foo(1,3),5));
    return 0;
}
