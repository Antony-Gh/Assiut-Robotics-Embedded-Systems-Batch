
/* Swap two integers with XOR; identical pointers are already in the desired state. */
#include <stdio.h>

void swap(int *a, int *b)
{
    if (a == NULL || b == NULL || a == b)
        return;

    *a ^= *b;
    *b ^= *a;
    *a ^= *b;
}

int main(void)
{
    int a = 10;
    int b = 20;

    swap(&a, &b);

    printf("a = %d\n", a);
    printf("b = %d\n", b);

    return 0;
}