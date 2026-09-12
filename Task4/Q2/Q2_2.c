
/* Task 4, Q2.2: multiply unsigned values using shifts and additions. */
#include <stdio.h>
#include <stdint.h>

/**
 * @brief Multiplies two unsigned integers using bitwise shifts and addition.
 * 
 * Also known as "Russian Peasant Multiplication". It shifts the first number left 
 * and the second number right, adding to the result when the second number's lowest bit is 1.
 * 
 * @param a The first unsigned integer.
 * @param b The second unsigned integer.
 * @return The product of a and b.
 */
uint32_t bitwise_multiply(uint32_t a, uint32_t b)
{
    uint32_t result = 0;

    while (b != 0)
    {
        if (b & 1u)
            result += a;

        a <<= 1;
        b >>= 1;
    }

    return result;
}

/**
 * @brief The main entry point of the program.
 * 
 * Tests the bitwise_multiply function.
 * 
 * @return 0 upon successful execution.
 */
int main(void)
{
    uint32_t a = 13;
    uint32_t b = 11;

    printf("%u * %u = %u\n",
           a, b, bitwise_multiply(a, b));

    return 0;
}