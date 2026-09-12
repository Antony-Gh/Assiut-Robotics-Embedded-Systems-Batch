
/* Task 4, Q3.1: define and use a parenthesized square macro. */
#include <stdio.h>

#define SQUARE(x) ((x) * (x))

int main(void)
{
    printf("Square of 5 = %d\n", SQUARE(5));
    printf("Square of 12.31 = %.4f\n",
           SQUARE(12.31));

    return 0;
}