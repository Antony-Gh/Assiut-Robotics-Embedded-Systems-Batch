
/* Task 5, Q1.3: sort an integer array with early-exit Bubble Sort. */
#include <stdio.h>
#include <stddef.h>

/* Bubble sort algorithm */
/*Complexity:

Case | Time Complexity

Best | O(n), with early stopping
Average | O(n²)
Worst| O(n²)

Space complexity: O(1), because sorting is performed in place. */

void bubble_sort(int arr[], size_t n)
{
    for (size_t i = 0; i + 1 < n; i++)
    {
        int swapped = 0;

        for (size_t j = 0; j + 1 < n - i; j++)
        {
            if (arr[j] > arr[j + 1])
            {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;

                swapped = 1;
            }
        }

        /* Already sorted */
        if (!swapped)
            break;
    }
}

int main(void)
{
    int arr[] = {5, 1, 4, 2, 8};
    size_t n = sizeof(arr) / sizeof(arr[0]);

    bubble_sort(arr, n);

    for (size_t i = 0; i < n; i++)
        printf("%d ", arr[i]);

    return 0;
}