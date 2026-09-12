
/* Task 5, Q3.1: use binary-search bounds to count values around a target. */
#include <stddef.h>
#include <stdio.h>


/**
 * @brief Finds the first position where a value could be inserted to maintain order.
 * 
 * Uses binary search to find the smallest index where arr[index] >= x.
 * 
 * @param arr The sorted array.
 * @param n The number of elements in the array.
 * @param x The target value.
 * @return The lower bound index.
 */
size_t lower_bound(const int arr[], size_t n, int x) {
  size_t left = 0;
  size_t right = n;

  while (left < right) {
    size_t mid = left + (right - left) / 2;

    if (arr[mid] < x)
      left = mid + 1;
    else
      right = mid;
  }

  return left;
}

/**
 * @brief Finds the first position after the target value.
 * 
 * Uses binary search to find the smallest index where arr[index] > x.
 * 
 * @param arr The sorted array.
 * @param n The number of elements in the array.
 * @param x The target value.
 * @return The upper bound index.
 */
size_t upper_bound(const int arr[], size_t n, int x) {
  size_t left = 0;
  size_t right = n;

  while (left < right) {
    size_t mid = left + (right - left) / 2;

    if (arr[mid] <= x)
      left = mid + 1;
    else
      right = mid;
  }

  return left;
}

/**
 * @brief The main entry point of the program.
 * 
 * Demonstrates using lower and upper bounds to find if an element exists, 
 * as well as counting how many elements are strictly less than or greater than 
 * a target value.
 * 
 * @return 0 upon successful execution.
 */
int main(void) {
  int arr[] = {1, 3, 5, 5, 7, 9};
  size_t n = sizeof(arr) / sizeof(arr[0]);
  int target = 5;

  size_t first = lower_bound(arr, n, target);
  size_t last = upper_bound(arr, n, target);

  if (first != last)
    printf("Element found\n");
  else
    printf("Element not found\n");

  printf("Less than %d: %zu\n", target, first);
  printf("Greater than %d: %zu\n", target, n - last);

  return 0;
}