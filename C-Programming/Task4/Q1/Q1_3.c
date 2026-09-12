
/* Task 4, Q1.3: count set bits and test whether a value is a power of two. */
#include <stdio.h>
#include <stdbool.h>

/**
 * @brief Counts the number of set bits (1s) in an unsigned integer.
 * 
 * Uses Brian Kernighan's algorithm which repeatedly clears the lowest set bit.
 * 
 * @param n The unsigned integer to evaluate.
 * @return The total count of bits that are set to 1.
 */
unsigned int count_set_bits(unsigned int n)
{
    unsigned int count = 0;

    while (n != 0)
    {
        n &= n - 1;
        count++;
    }

    return count;
}

/**
 * @brief Determines if an unsigned integer is a power of two.
 * 
 * A power of two has exactly one bit set to 1. The expression (n & (n - 1)) 
 * clears the lowest set bit, so for a power of two, the result must be 0.
 * 
 * @param n The unsigned integer to check.
 * @return true if 'n' is a power of two, false otherwise.
 */
bool is_power_of_two(unsigned int n)
{
    return n != 0 && (n & (n - 1)) == 0;
}

/**
 * @brief The main entry point of the program.
 * 
 * Tests the count_set_bits and is_power_of_two functions.
 * 
 * @return 0 upon successful execution.
 */
int main(void)
{
    unsigned int n = 16;

    printf("Set bits: %u\n", count_set_bits(n));
    printf("Power of 2: %s\n",
           is_power_of_two(n) ? "Yes" : "No");

    return 0;
}