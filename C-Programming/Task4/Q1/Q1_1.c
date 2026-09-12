
/* Task 4, Q1.1: determine parity with the least-significant bit. */
#include <stdio.h>

/**
 * @brief Checks if a number is even or odd using bitwise operations.
 * 
 * Uses the bitwise AND operator '&' with 1. If the least significant bit is 0,
 * the number is even; if it is 1, the number is odd.
 * 
 * @param n The integer to check.
 */
void check_even_odd(int n)
{
    if ((n & 1) == 0)
        printf("%d is even\n", n);
    else
        printf("%d is odd\n", n);
}

/**
 * @brief The main entry point of the program.
 * 
 * Tests the check_even_odd function with several sample integers.
 * 
 * @return 0 upon successful execution.
 */
int main(void)
{
    check_even_odd(10);
    check_even_odd(7);
    check_even_odd(-4);
    check_even_odd(-7);

    return 0;
}