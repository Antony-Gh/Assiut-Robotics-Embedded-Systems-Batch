
/* Task 4, Q2.1: reverse the significant bits of an unsigned integer. */
#include <stdio.h>
#include <stdint.h>

uint32_t reverse_significant_bits(uint32_t x)
{
    uint32_t result = 0;

    while (x != 0)
    {
        result = (result << 1) | (x & 1u);
        x >>= 1;
    }

    return result;
}

int main(void)
{
    uint32_t x = 11;

    printf("Reversed: %u\n",
           reverse_significant_bits(x));

    return 0;
}