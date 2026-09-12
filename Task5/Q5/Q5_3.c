
/* Task 5, Q5.3: count sorted-array occurrences from binary-search bounds. */
#include <stddef.h>
#include <stdio.h>


/**
 * @brief Finds the first occurrence of an element in a sorted array using binary search.
 * 
 * @param arr The sorted array to search in.
 * @param n The number of elements in the array.
 * @param x The target value to search for.
 * @return The index of the first element not less than x, or n if all are less.
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
 * @brief Finds the first element strictly greater than the target in a sorted array.
 * 
 * @param arr The sorted array to search in.
 * @param n The number of elements in the array.
 * @param x The target value to search for.
 * @return The index of the first element strictly greater than x, or n if none exist.
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
 * @brief Counts the total occurrences of a target element in a sorted array.
 * 
 * Uses the difference between upper_bound and lower_bound to efficiently find the count.
 * 
 * @param arr The sorted array.
 * @param n The number of elements in the array.
 * @param target The target value to count.
 * @return The number of times the target value appears in the array.
 */
size_t count_occurrences(const int arr[], size_t n, int target) {
  return upper_bound(arr, n, target) - lower_bound(arr, n, target);
}

/**
 * @brief The main entry point of the program.
 * 
 * Demonstrates counting occurrences of a target number in a sorted array.
 * 
 * @return 0 upon successful execution.
 */
int main(void) {
  int arr[] = {1, 2, 2, 2, 3, 4, 5};
  size_t n = sizeof(arr) / sizeof(arr[0]);

  printf("Occurrences of 2: %zu\n", count_occurrences(arr, n, 2));

  return 0;
}