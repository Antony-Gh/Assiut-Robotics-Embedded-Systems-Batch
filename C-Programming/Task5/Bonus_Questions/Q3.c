
/* Task 5 bonus: sort a copy and scan for the minimum excluded value. */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>


/**
 * @brief Merges two sorted halves of an array into a temporary array.
 * 
 * @param arr The original array containing the halves.
 * @param temp The temporary array used for merging.
 * @param left The starting index of the left half.
 * @param mid The starting index of the right half.
 * @param right The ending index (exclusive) of the right half.
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
 * @brief Recursively splits the array for Merge Sort.
 * 
 * @param arr The array to sort.
 * @param temp A temporary array used for merging.
 * @param left The starting index of the range to sort.
 * @param right The ending index (exclusive) of the range.
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
 * @brief Sorts an array using the Merge Sort algorithm.
 * 
 * @param arr The array to sort.
 * @param n The total number of elements.
 * @return 1 on success, 0 on failure.
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
 * @brief Finds the Minimum Excluded (MEX) value in an array.
 * 
 * It makes a copy of the array, sorts the copy, and then iterates through 
 * to find the smallest non-negative integer that is missing.
 * 
 * @param arr The array of integers.
 * @param n The total number of elements.
 * @return The MEX value, or -1 on memory allocation error.
 */
int mex(int arr[], size_t n) {
  if (n > (size_t)-1 / sizeof(int))
    return -1;

  int *copy = malloc(n * sizeof *copy);

  if (n != 0 && copy == NULL)
    return -1;

  for (size_t i = 0; i < n; i++)
    copy[i] = arr[i];

  if (!merge_sort(copy, n)) {
    free(copy);
    return -1;
  }

  int expected = 0;

  for (size_t i = 0; i < n; i++) {
    if (copy[i] < expected)
      continue;

    if (copy[i] == expected)
      expected++;
    else
      break;
  }

  free(copy);
  return expected;
}

/**
 * @brief The main entry point of the program.
 * 
 * Demonstrates finding the MEX value for an array.
 * 
 * @return 0 upon successful execution.
 */
int main(void) {
  int arr[] = {0, 1, 3, 5};
  size_t n = sizeof(arr) / sizeof(arr[0]);

  printf("MEX = %d\n", mex(arr, n));

  return 0;
}