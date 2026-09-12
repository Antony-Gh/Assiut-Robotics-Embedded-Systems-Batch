
/* Task 5, Q2.3: compute Fibonacci numbers iteratively with constant space. */
#include <stdio.h>
#include <stdint.h>


/* Fibonacci algorithm */
/*
Time complexity: O(n).
Space complexity: O(1).
*/
/**
 * @brief Computes the nth Fibonacci number iteratively.
 * 
 * Uses a constant amount of memory (O(1) space) and computes the number 
 * in linear time (O(n)).
 * 
 * @param n The position of the Fibonacci number to compute.
 * @return The nth Fibonacci number.
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

/**
 * @brief The main entry point of the program.
 * 
 * Tests the fibonacci function by computing the 10th Fibonacci number.
 * 
 * @return 0 upon successful execution.
 */
int main(void)
{
    printf("Fibonacci(10) = %llu\n",
           (unsigned long long)fibonacci(10));

    return 0;
}