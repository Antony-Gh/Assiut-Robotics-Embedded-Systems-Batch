
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