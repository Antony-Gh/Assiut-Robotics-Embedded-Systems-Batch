
/* Task 4, Q3.2: swap values of a caller-selected type through a macro. */
#include <stdio.h>

#define SWAP(a, b, type)       \
    do                         \
    {                          \
        type temp = (a);       \
        (a) = (b);             \
        (b) = temp;            \
    } while (0)

/**
 * @brief The main entry point of the program.
 * 
 * Tests the SWAP macro on different data types (int and double).
 * 
 * @return 0 upon successful execution.
 */
int main(void)
{
    int x = 10, y = 20;
    double a = 1.5, b = 2.5;

    SWAP(x, y, int);
    SWAP(a, b, double);

    printf("x = %d, y = %d\n", x, y);
    printf("a = %.1f, b = %.1f\n", a, b);

    return 0;
}