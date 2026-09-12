/* Task 1, Q3: clear, set, and toggle a selected bit using masks. */
#include <stdio.h>

#include <limits.h>

/**
 * @brief Performs bitwise operations to clear, set, and toggle a specific bit.
 * 
 * This function takes a number and a bit position 'n', then computes three new values:
 * - cleared: The original number with the n-th bit set to 0.
 * - set: The original number with the n-th bit set to 1.
 * - toggled: The original number with the n-th bit flipped (0 becomes 1, 1 becomes 0).
 * 
 * @param number The input integer on which operations will be performed.
 * @param n The bit position to manipulate (0-indexed).
 * @param cleared Pointer to store the result after clearing the bit.
 * @param set Pointer to store the result after setting the bit.
 * @param toggled Pointer to store the result after toggling the bit.
 * @return 1 if successful, 0 if the bit position is invalid or pointers are NULL.
 */
int bitOperations(int number, unsigned int n, unsigned int *cleared,
                  unsigned int *set, unsigned int *toggled) {
  if (n >= sizeof(unsigned int) * CHAR_BIT || cleared == NULL || set == NULL ||
      toggled == NULL)
    return 0;

  unsigned int value = (unsigned int)number;
  unsigned int mask = 1u << n;

  *cleared = value & ~mask;
  *set = value | mask;
  *toggled = value ^ mask;

  return 1;
}

/**
 * @brief The main entry point of the program.
 * 
 * This function prompts the user to enter an integer and a bit position.
 * It reads the inputs, calls the bitOperations function to perform calculations,
 * and prints the results (cleared, set, and toggled values).
 * 
 * @return 0 upon successful execution, 1 if input parsing fails or bit position is invalid.
 */
int main(void) {
  int number;
  unsigned int n, cleared, set, toggled;

  printf("Enter an integer and bit position: ");
  if (scanf("%d %u", &number, &n) != 2)
    return 1;

  if (!bitOperations(number, n, &cleared, &set, &toggled)) {
    printf("Invalid bit position.\n");
    return 1;
  }

  printf("Cleared: %u\n", cleared);
  printf("Set:     %u\n", set);
  printf("Toggled: %u\n", toggled);

  return 0;
}