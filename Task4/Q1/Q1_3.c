
#include <stdio.h>
#include <stdbool.h>

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

bool is_power_of_two(unsigned int n)
{
    return n != 0 && (n & (n - 1)) == 0;
}

int main(void)
{
    unsigned int n = 16;

    printf("Set bits: %u\n", count_set_bits(n));
    printf("Power of 2: %s\n",
           is_power_of_two(n) ? "Yes" : "No");

    return 0;
}