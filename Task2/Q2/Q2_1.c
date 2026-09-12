#include <stdio.h>

int arrayStats(const int arr[], int size, int *min, int *max, double *average) {
  if (arr == NULL || size <= 0 || min == NULL || max == NULL || average == NULL)
    return 0;

  long long sum = 0;
  *min = arr[0];
  *max = arr[0];

  for (int i = 0; i < size; i++) {
    if (arr[i] < *min)
      *min = arr[i];

    if (arr[i] > *max)
      *max = arr[i];

    sum += arr[i];
  }

  *average = (double)sum / size;
  return 1;
}

int main(void) {
  int numbers[] = {-5, 0, 10, 15, 20, 70};
  int min, max;
  double average;

  if (arrayStats(numbers, 6, &min, &max, &average))
    printf("Min=%d, Max=%d, Average=%.2f\n", min, max, average);
}