/* Task 2, Q1.1: exchange two integers through pointer parameters. */
#include <stdio.h>

/**
 * @brief Swaps the values of two integers using pointers.
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
 * @brief The main entry point of the program.
 * 
 * Demonstrates the swap function by exchanging the values of two variables.
 * 
 * @return 0 upon successful execution.
 */
int main(void) {
  int x = 10, y = 20;
  swap(&x, &y);
  printf("x = %d, y = %d\n", x, y);
  return 0;
}