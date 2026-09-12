
/* Task 5, Q2.1: locate the first target occurrence with Linear Search. */
#include <stdio.h>
#include <stddef.h>

/* Linear search algorithm */
/*
Time complexity: O(n) worst case, O(1) best case.
Space complexity: O(1).
*/
/**
 * @brief Searches for a target value in an array using Linear Search.
 * 
 * Iterates through each element one by one until the target is found.
 * 
 * @param arr The array to search in.
 * @param n The number of elements in the array.
 * @param target The value to search for.
 * @return The index of the first occurrence of the target, or -1 if not found.
 */
int linear_search(const int arr[], size_t n, int target)
{
    for (size_t i = 0; i < n; i++)
    {
        if (arr[i] == target)
            return (int)i;
    }

    return -1;
}

/**
 * @brief The main entry point of the program.
 * 
 * Demonstrates finding an element using linear search and prints its index.
 * 
 * @return 0 upon successful execution.
 */
int main(void)
{
    int arr[] = {10, 20, 30, 40, 50};
    size_t n = sizeof(arr) / sizeof(arr[0]);

    int index = linear_search(arr, n, 30);

    if (index != -1)
        printf("Found at index %d\n", index);
    else
        printf("Not found\n");

    return 0;
}