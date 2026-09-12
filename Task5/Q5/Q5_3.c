
/* Task 5, Q5.3: count sorted-array occurrences from binary-search bounds. */
#include <stddef.h>
#include <stdio.h>


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

size_t count_occurrences(const int arr[], size_t n, int target) {
  return upper_bound(arr, n, target) - lower_bound(arr, n, target);
}

int main(void) {
  int arr[] = {1, 2, 2, 2, 3, 4, 5};
  size_t n = sizeof(arr) / sizeof(arr[0]);

  printf("Occurrences of 2: %zu\n", count_occurrences(arr, n, 2));

  return 0;
}