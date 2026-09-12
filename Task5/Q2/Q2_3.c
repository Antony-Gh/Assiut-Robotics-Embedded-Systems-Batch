
#include <stdio.h>
#include <stdint.h>


/* Fibonacci algorithm */
/*
Time complexity: O(n).
Space complexity: O(1).
*/
uint64_t fibonacci(unsigned int n)
{
    uint64_t a = 0;
    uint64_t b = 1;

    for (unsigned int i = 0; i < n; i++)
    {
        uint64_t next = a + b;
        a = b;
        b = next;
    }

    return a;
}

int main(void)
{
    printf("Fibonacci(10) = %llu\n",
           (unsigned long long)fibonacci(10));

    return 0;
}