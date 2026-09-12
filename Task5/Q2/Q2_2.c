
/* Task 5, Q2.2: compare in-place Insertion Sort and Selection Sort. */
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

/* Insertion sort algorithm */
/*
Complexity comparison:
Algorithm | Best time | Average / worst time | Extra space
Insertion Sort | O(n) | O(n²) | O(1)
Selection Sort | O(n²) | O(n²) | O(1)

Insertion Sort is efficient for small or nearly sorted arrays. Selection Sort performs fewer swaps, but still performs O(n²) comparisons.
*/
/**
 * @brief Sorts an array using the Insertion Sort algorithm.
 * 
 * Builds the final sorted array one item at a time. It is much less efficient 
 * on large lists than more advanced algorithms such as quicksort, heapsort, 
 * or merge sort, but is very efficient for small data sets.
 * 
 * @param arr The array to sort.
 * @param n The number of elements.
 */
void insertion_sort(int arr[], size_t n)
{
    for (size_t i = 1; i < n; i++)
    {
        int key = arr[i];
        size_t j = i;

        while (j > 0 && arr[j - 1] > key)
        {
            arr[j] = arr[j - 1];
            j--;
        }

        arr[j] = key;
    }
}

/**
 * @brief Sorts an array using the Selection Sort algorithm.
 * 
 * Sorts an array by repeatedly finding the minimum element from the unsorted 
 * part and putting it at the beginning.
 * 
 * @param arr The array to sort.
 * @param n The number of elements.
 */
void selection_sort(int arr[], size_t n)
{
    for (size_t i = 0; i + 1 < n; i++)
    {
        size_t min_index = i;

        for (size_t j = i + 1; j < n; j++)
        {
            if (arr[j] < arr[min_index])
                min_index = j;
        }

        if (min_index != i)
        {
            int temp = arr[i];
            arr[i] = arr[min_index];
            arr[min_index] = temp;
        }
    }
}

/**
 * @brief The main entry point of the program.
 * 
 * Tests and compares both insertion_sort and selection_sort on an array.
 * 
 * @return 0 upon successful execution.
 */
int main(void)
{
    int arr[] = {5, 2, 4, 6, 1, 3};
    int arr2[] = {5, 2, 4, 6, 1, 3};

    size_t n = sizeof(arr) / sizeof(arr[0]);

    insertion_sort(arr, n);

    for (size_t i = 0; i < n; i++)
        printf("%d ", arr[i]);

    printf("\n");

    selection_sort(arr2, n);

    for (size_t i = 0; i < n; i++)
        printf("%d ", arr2[i]);

    printf("\n");

    return 0;
}