
/* Task 4, Q2.3: find a missing value using XOR cancellation. */
#include <stdio.h>
#include <stddef.h>

/**
 * @brief Finds a missing number in an array containing numbers from 0 to n.
 * 
 * Uses the property of XOR (A ^ A = 0). By XORing all indices and all array 
 * values together, all the present numbers cancel out, leaving only the missing one.
 * 
 * @param arr The array of integers.
 * @param size The number of elements in the array.
 * @return The missing integer.
 */
int find_missing(const int arr[], size_t size)
{
    int missing = (int)size;

    for (size_t i = 0; i < size; i++)
    {
        missing ^= (int)i;
        missing ^= arr[i];
    }

    return missing;
}

/**
 * @brief The main entry point of the program.
 * 
 * Demonstrates finding the missing number in an array using XOR.
 * 
 * @return 0 upon successful execution.
 */
int main(void)
{
    int arr[] = {3, 0, 1};
    size_t size = sizeof(arr) / sizeof(arr[0]);

    printf("Missing number: %d\n",
           find_missing(arr, size));

    return 0;
}