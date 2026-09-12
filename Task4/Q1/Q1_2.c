
/* Swap two integers with XOR; identical pointers are already in the desired state. */
#include <stdio.h>

/**
 * @brief Swaps two integers without using a temporary variable.
 * 
 * Uses the bitwise XOR '^' operator to swap the values. Note that it skips 
 * swapping if the pointers are identical to avoid zeroing out the value.
 * 
 * @param a Pointer to the first integer.
 * @param b Pointer to the second integer.
 */
void swap(int *a, int *b)
{
    if (a == NULL || b == NULL || a == b)
        return;

    *a ^= *b;
    *b ^= *a;
    *a ^= *b;
}

/**
 * @brief The main entry point of the program.
 * 
 * Demonstrates the XOR swap function on two integer variables.
 * 
 * @return 0 upon successful execution.
 */
int main(void)
{
    int a = 10;
    int b = 20;

    swap(&a, &b);

    printf("a = %d\n", a);
    printf("b = %d\n", b);

    return 0;
}