/* Task 2, Q2.1: calculate array minimum, maximum, and average in one pass. */
#include <stdio.h>

/**
 * @brief Calculates the minimum, maximum, and average of an integer array.
 * 
 * This function iterates through the array exactly once to find the minimum
 * and maximum values, while simultaneously calculating the sum to find the average.
 * 
 * @param arr The array of integers to process.
 * @param size The number of elements in the array.
 * @param min Pointer to store the minimum value found.
 * @param max Pointer to store the maximum value found.
 * @param average Pointer to store the computed average.
 * @return 1 if successful, 0 if pointers are NULL or size is invalid.
 */
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

/**
 * @brief The main entry point of the program.
 * 
 * Initializes an array of integers, then uses the arrayStats function
 * to calculate its minimum, maximum, and average, and prints the results.
 * 
 * @return 0 upon successful execution.
 */
int main(void) {
  int numbers[] = {-5, 0, 10, 15, 20, 70};
  int min, max;
  double average;

  if (arrayStats(numbers, 6, &min, &max, &average))
    printf("Min=%d, Max=%d, Average=%.2f\n", min, max, average);
}