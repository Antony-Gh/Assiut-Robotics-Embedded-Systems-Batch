
/* Task 4, Q3.1: define and use a parenthesized square macro. */
#include <stdio.h>

#define SQUARE(x) ((x) * (x))

/**
 * @brief The main entry point of the program.
 * 
 * Demonstrates the use of the SQUARE macro to calculate the square of a number.
 * 
 * @return 0 upon successful execution.
 */
int main(void)
{

    int x = 10.0;
    float y = 7.25;

    int v = ((float)x)*y;
    printf("Square of 5 = %d\n", SQUARE(5));
    printf("Square of 12.31 = %.4f\n", SQUARE(12.31));

    return 0;
}