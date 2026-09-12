
/* Task 4 bonus: find two values that occur once using XOR partitioning. */
#include <stddef.h>
#include <stdio.h>


/**
 * @brief Finds two non-repeating elements in an array using XOR partitioning.
 * 
 * First, it XORs all elements to find the XOR sum of the two unique elements.
 * Then, it isolates the rightmost set bit in this sum to divide the elements 
 * into two groups, each containing one of the unique elements.
 * 
 * @param arr The array of integers.
 * @param size The number of elements in the array.
 * @param first Pointer to store the first unique element.
 * @param second Pointer to store the second unique element.
 */
void find_two_unique(const int arr[], size_t size, int *first, int *second) {
  int xor_result = 0;

  for (size_t i = 0; i < size; i++)
    xor_result ^= arr[i];

  /* Isolate the rightmost set bit */
  unsigned int diff =
      (unsigned int)xor_result & (0u - (unsigned int)xor_result);

  int a = 0;
  int b = 0;

  for (size_t i = 0; i < size; i++) {
    if (((unsigned int)arr[i] & diff) != 0)
      a ^= arr[i];
    else
      b ^= arr[i];
  }

  *first = a;
  *second = b;
}

/**
 * @brief The main entry point of the program.
 * 
 * Demonstrates finding two unique elements in an array where all other 
 * elements appear exactly twice.
 * 
 * @return 0 upon successful execution.
 */
int main(void) {
  int arr[] = {2, 4, 7, 9, 2, 4};
  size_t size = sizeof(arr) / sizeof(arr[0]);

  int first, second;

  find_two_unique(arr, size, &first, &second);

  printf("Unique elements: %d and %d\n", first, second);

  return 0;
}