
/* Task 4, Q1.4: find the sole non-repeating array value with XOR. */
#include <stdio.h>
#include <stddef.h>

/**
 * @brief Finds the only non-repeating element in an array.
 * 
 * Since XORing a number with itself results in 0 (A ^ A = 0), XORing all 
 * elements together cancels out all repeating elements, leaving only the unique one.
 * 
 * @param arr The array of integers.
 * @param size The number of elements in the array.
 * @return The single non-repeating integer.
 */
int find_single(const int arr[], size_t size)
{
    int result = 0;

    for (size_t i = 0; i < size; i++)
        result ^= arr[i];

    return result;
}

/**
 * @brief The main entry point of the program.
 * 
 * Demonstrates finding the unique non-repeating element in an array using XOR.
 * 
 * @return 0 upon successful execution.
 */
int main(void)
{
    int arr[] = {4, 1, 2, 1, 2};
    size_t size = sizeof(arr) / sizeof(arr[0]);

    printf("Unique element: %d\n",
           find_single(arr, size));

    return 0;
}