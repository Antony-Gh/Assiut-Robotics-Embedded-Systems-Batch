/* Task 1, Q3: clear, set, and toggle a selected bit using masks. */
#include <stdio.h>

#include <limits.h>

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