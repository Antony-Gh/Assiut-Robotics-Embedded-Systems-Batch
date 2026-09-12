/* Task 2, Q3: sort an integer array with early-exit Bubble Sort. */
#include <stdio.h>

/**
 * @brief Swaps the values of two integer variables.
 * 
 * @param a Pointer to the first integer.
 * @param b Pointer to the second integer.
 */
void swap(int *a, int *b) {
  int temp = *a;
  *a = *b;
  *b = temp;
}

/**
 * @brief Sorts an array using the Bubble Sort algorithm.
 * 
 * Repeatedly steps through the array, compares adjacent elements, and swaps 
 * them if they are in the wrong order. It includes an early-exit optimization:
 * if a pass finishes without any swaps, the array is sorted and it stops.
 * 
 * @param arr The array to sort.
 * @param size The number of elements in the array.
 */
void bubbleSort(int arr[], int size)
{
    for (int i = 0; i < size - 1; i++)
    {
        int swapped = 0;

        for (int j = 0; j < size - 1 - i; j++)
        {
            if (arr[j] > arr[j + 1])
            {
                swap(&arr[j], &arr[j + 1]);
                swapped = 1;
            }
        }

        if (!swapped)
            break;
    }
}

/**
 * @brief The main entry point of the program.
 * 
 * Initializes an array, sorts it using bubbleSort, and prints the sorted array.
 * 
 * @return 0 upon successful execution.
 */
int main(void) {
  int arr[] = {5, 3, 8, 4, 2};
  int size = sizeof(arr) / sizeof(arr[0]);
  bubbleSort(arr, size);
  for (int i = 0; i < size; i++) {
    printf("%d ", arr[i]);
  }
  return 0;
}