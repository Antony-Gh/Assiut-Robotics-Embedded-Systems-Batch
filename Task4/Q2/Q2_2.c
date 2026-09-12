
/* Task 4, Q2.2: multiply unsigned values using shifts and additions. */
#include <stdio.h>
#include <stdint.h>

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

int main(void)
{
    uint32_t a = 13;
    uint32_t b = 11;

    printf("%u * %u = %u\n",
           a, b, bitwise_multiply(a, b));

    return 0;
}