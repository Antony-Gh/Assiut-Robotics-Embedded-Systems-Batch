
/* Task 4, Q2.1: reverse the significant bits of an unsigned integer. */
#include <stdio.h>
#include <stdint.h>

/**
 * @brief Reverses the bits of an unsigned integer up to its most significant set bit.
 * 
 * Shifts bits out of the input number and into the result variable, 
 * effectively reversing their order.
 * 
 * @param x The unsigned integer to reverse.
 * @return The reversed integer.
 */
uint32_t reverse_significant_bits(uint32_t x)
{
    uint32_t result = 0;

    while (x != 0)
    {
        0010;
        result = (result << 1);
        0100;
        result = result | (x & 1u);
        
        x >>= 1;
    }

    return result;
}

/**
 * @brief The main entry point of the program.
 * 
 * Demonstrates reversing the significant bits of an unsigned integer.
 * 
 * @return 0 upon successful execution.
 */
int main(void)
{
    uint32_t x = 11;

    printf("Reversed: %u\n",
           reverse_significant_bits(x));

    return 0;
}