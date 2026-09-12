
/* Task 5 bonus: combine merge sort with a two-pointer target-sum search. */
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

int find_pair(int arr[], size_t n, int target, int *first, int *second) {
  if (n < 2)
    return 0;

  if (!merge_sort(arr, n))
    return 0;

  size_t left = 0;
  size_t right = n - 1;

  while (left < right) {
    long long sum = (long long)arr[left] + arr[right];

    if (sum == target) {
      *first = arr[left];
      *second = arr[right];
      return 1;
    }

    if (sum < target)
      left++;
    else
      right--;
  }

  return 0;
}

int main(void) {
  int arr[] = {8, 3, 4, 7, 2};
  size_t n = sizeof(arr) / sizeof(arr[0]);

  int target = 11;
  int first, second;

  if (find_pair(arr, n, target, &first, &second)) {
    printf("Pair: %d + %d = %d\n", first, second, target);
  } else {
    printf("No pair found\n");
  }

  return 0;
}