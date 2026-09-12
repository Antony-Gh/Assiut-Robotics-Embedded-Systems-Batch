/* Reverse a user-provided integer array in place using two moving indices. */
#include <stdio.h>

/**
 * @brief Swaps the values of two integer variables.
 * 
 * This function takes pointers to two integers and exchanges their values.
 * 
 * @param a Pointer to the first integer.
 * @param b Pointer to the second integer.
 */
void swap(int *a, int *b) {
  int temp = *a;
  *a = *b;
  *b = temp;
}

/**
 * @brief Reverses the elements of an integer array in place.
 * 
 * Uses two moving indices (left starting at 0, right starting at the end) 
 * to swap elements until they meet in the middle, effectively reversing the array.
 * 
 * @param arr The array to be reversed.
 * @param size The number of elements in the array.
 */
void reverseArray(int arr[], int size) {
  int left = 0;
  int right = size - 1;

  while (left < right) {
    swap(&arr[left], &arr[right]);
    left++;
    right--;
  }
}

/**
 * @brief The main entry point of the program.
 * 
 * Prompts the user for the size of an array and its elements.
 * Then, it reverses the array in place using the reverseArray function
 * and prints the reversed elements.
 * 
 * @return 0 upon successful execution, 1 if input fails.
 */
int main(void) {
  int size;

  printf("Enter array size: ");
  if (scanf("%d", &size) != 1 || size <= 0)
    return 1;

  int arr[size];

  for (int i = 0; i < size; i++) {
    printf("Enter element %d: ", i + 1);
    if (scanf("%d", &arr[i]) != 1)
      return 1;
  }

  reverseArray(arr, size);

  printf("Reversed array: ");
  for (int i = 0; i < size; i++)
    printf("%d ", arr[i]);

  printf("\n");
  return 0;
}