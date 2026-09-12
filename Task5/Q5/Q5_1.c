
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>


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

void merge_sort_recursive(int arr[], int temp[], size_t left, size_t right) {
  if (right - left <= 1)
    return;

  size_t mid = left + (right - left) / 2;

  merge_sort_recursive(arr, temp, left, mid);
  merge_sort_recursive(arr, temp, mid, right);

  merge(arr, temp, left, mid, right);
}

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