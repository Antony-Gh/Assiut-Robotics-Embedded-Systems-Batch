
/* Task 5, Q3.2: search sorted data by blocks followed by a linear scan. */
#include <stdio.h>
#include <stddef.h>

/* Return floor(sqrt(n)) without multiplication overflow. */
/**
 * @brief Calculates the integer square root of a number without multiplication overflow.
 * 
 * @param n The number to find the square root of.
 * @return The floor of the square root.
 */
size_t integer_sqrt(size_t n)
{
    size_t root = 0;

    while (root + 1 <= n / (root + 1))
        root++;

    return root;
}

/**
 * @brief Searches for a target in a sorted array using Jump Search.
 * 
 * Jumps ahead by fixed steps (sqrt(n)) until it finds a block where the target 
 * might be, then performs a linear search within that block.
 * 
 * @param arr The sorted array.
 * @param n The number of elements.
 * @param target The value to search for.
 * @return The index of the target if found, otherwise -1.
 */
int jump_search(const int arr[], size_t n, int target)
{
    if (n == 0)
        return -1;

    size_t step = integer_sqrt(n);

    if (step == 0)
        step = 1;

    size_t start = 0;
    size_t end = step - 1;

    while (start < n)
    {
        if (arr[end] >= target)
            break;

        start = end + 1;

        if (start >= n)
            return -1;

        if (step - 1 > n - 1 - start)
            end = n - 1;
        else
            end = start + step - 1;
    }

    for (size_t i = start; i <= end; i++)
    {
        if (arr[i] == target)
            return (int)i;

        if (arr[i] > target)
            break;
    }

    return -1;
}

/**
 * @brief The main entry point of the program.
 * 
 * Tests the jump_search function on a sorted array.
 * 
 * @return 0 upon successful execution.
 */
int main(void)
{
    int arr[] = {1, 3, 5, 7, 9, 11, 13, 15};
    size_t n = sizeof(arr) / sizeof(arr[0]);

    int target = 11;
    int index = jump_search(arr, n, target);

    if (index != -1)
        printf("Found at index %d\n", index);
    else
        printf("Not found\n");

    return 0;
}