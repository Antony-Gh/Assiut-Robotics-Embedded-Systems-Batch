
#include <stddef.h>
#include <stdio.h>


#define Max_arr_macro(arr, size)                                               \
  ({                                                                           \
    int max_value = (arr)[0];                                                  \
    for (size_t i = 1; i < (size); i++) {                                      \
      if ((arr)[i] > max_value)                                                \
        max_value = (arr)[i];                                                  \
    }                                                                          \
    max_value;                                                                 \
  })

/**
 * @brief Finds the maximum value in an integer array.
 * 
 * @param arr The array of integers.
 * @param size The number of elements in the array.
 * @return The maximum integer found.
 */
int max_array(const int arr[], size_t size) {
  if (size == 0)
    return 0;

  int max_value = arr[0];

  for (size_t i = 1; i < size; i++) {
    if (arr[i] > max_value)
      max_value = arr[i];
  }

  return max_value;
}

#define Max_arr(arr, size) max_array((arr), (size))

/**
 * @brief The main entry point of the program.
 * 
 * Demonstrates finding the maximum value in an array using the Max_arr macro.
 * 
 * @return 0 upon successful execution.
 */
int main(void) {
  int arr[] = {10, 5, 25, 3, 18};
  size_t size = sizeof(arr) / sizeof(arr[0]);

  printf("Maximum = %d\n", Max_arr(arr, size));

  return 0;
}