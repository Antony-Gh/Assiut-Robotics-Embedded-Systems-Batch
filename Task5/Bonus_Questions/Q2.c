
#include <stddef.h>
#include <stdio.h>


int find_missing(const int arr[], size_t n) {
  int result = 0;

  for (size_t i = 1; i <= n + 1; i++)
    result ^= (int)i;

  for (size_t i = 0; i < n; i++)
    result ^= arr[i];

  return result;
}

int main(void) {
  int arr[] = {1, 2, 4, 5};
  size_t n = sizeof(arr) / sizeof(arr[0]);

  printf("Missing number: %d\n", find_missing(arr, n));

  return 0;
}