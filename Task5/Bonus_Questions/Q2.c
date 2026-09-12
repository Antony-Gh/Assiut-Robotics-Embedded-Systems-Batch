
/* Task 5 bonus: find a missing value with XOR in linear time. */
#include <stddef.h>
#include <stdio.h>


/**
 * @brief Finds a missing number in a sequence from 1 to n+1 using XOR.
 * 
 * Because A ^ A = 0, XORing all indices and all array elements together 
 * cancels out numbers present in the array, leaving only the missing one.
 * 
 * @param arr The array of integers.
 * @param n The number of elements in the array.
 * @return The missing integer.
 */
int find_missing(const int arr[], size_t n) {
  int result = 0;

  for (size_t i = 1; i <= n + 1; i++)
    result ^= (int)i;

  for (size_t i = 0; i < n; i++)
    result ^= arr[i];

  return result;
}

/**
 * @brief The main entry point of the program.
 * 
 * Tests the find_missing function.
 * 
 * @return 0 upon successful execution.
 */
int main(void) {
  int arr[] = {1, 2, 4, 5};
  size_t n = sizeof(arr) / sizeof(arr[0]);

  printf("Missing number: %d\n", find_missing(arr, n));

  return 0;
}