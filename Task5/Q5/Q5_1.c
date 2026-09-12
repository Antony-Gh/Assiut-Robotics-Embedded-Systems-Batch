
/* Task 5, Q5.1: sort an integer array with divide-and-conquer Merge Sort. */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>


/**
 * @brief Merges two sorted sub-arrays into a single sorted array.
 * 
 * @param arr The original array containing the two sub-arrays.
 * @param temp A temporary buffer array used for merging.
 * @param left The starting index of the left sub-array.
 * @param mid The ending index of the left sub-array (and start of the right).
 * @param right The ending index of the right sub-array.
 */
void merge(int arr[], int temp[], size_t left, size_t mid, size_t right) {
  size_t i = left;
  size_t j = mid;
  size_t k = left;

  while (i < mid && j < right) {
    if (arr[i] <= arr[j])
      temp[k++] = arr[i++];
    else
      temp[k++] = arr[j++];
  }

  while (i < mid)
    temp[k++] = arr[i++];

  while (j < right)
    temp[k++] = arr[j++];

  for (size_t p = left; p < right; p++)
    arr[p] = temp[p];
}

/**
 * @brief Recursively divides the array and sorts it using merge sort.
 * 
 * @param arr The array to sort.
 * @param temp A temporary array used for merging.
 * @param left The starting index of the sub-array.
 * @param right The ending index of the sub-array.
 */
void merge_sort_recursive(int arr[], int temp[], size_t left, size_t right) {
  if (right - left <= 1)
    return;

  size_t mid = left + (right - left) / 2;

  merge_sort_recursive(arr, temp, left, mid);
  merge_sort_recursive(arr, temp, mid, right);

  merge(arr, temp, left, mid, right);
}

/**
 * @brief Sorts an integer array using the Merge Sort algorithm.
 * 
 * This is the public wrapper function that allocates the required temporary 
 * memory buffer before kicking off the recursive sort.
 * 
 * @param arr The array of integers to sort.
 * @param n The number of elements in the array.
 * @return 1 on success, 0 on memory allocation failure or invalid size.
 */
int merge_sort(int arr[], size_t n) {
  if (n < 2)
    return 1;

  if (n > (size_t)-1 / sizeof(int))
    return 0;

  int *temp = malloc(n * sizeof *temp);

  if (temp == NULL)
    return 0;

  merge_sort_recursive(arr, temp, 0, n);

  free(temp);
  return 1;
}

/**
 * @brief The main entry point of the program.
 * 
 * Initializes an array of integers, sorts them using merge_sort, 
 * and prints the sorted output.
 * 
 * @return 0 upon successful execution, 1 on error.
 */
int main(void) {
  int arr[] = {38, 27, 43, 3, 9, 82, 10};
  size_t n = sizeof(arr) / sizeof(arr[0]);

  if (!merge_sort(arr, n)) {
    printf("Memory allocation failed\n");
    return 1;
  }

  for (size_t i = 0; i < n; i++)
    printf("%d ", arr[i]);

  printf("\n");

  return 0;
}