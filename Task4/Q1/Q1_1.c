
/* Task 4, Q1.1: determine parity with the least-significant bit. */
#include <stdio.h>

void check_even_odd(int n)
{
    if ((n & 1) == 0)
        printf("%d is even\n", n);
    else
        printf("%d is odd\n", n);
}

int main(void)
{
    check_even_odd(10);
    check_even_odd(7);
    check_even_odd(-4);
    check_even_odd(-7);

    return 0;
}