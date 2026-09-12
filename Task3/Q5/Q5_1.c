/* Task 3, Q5.1: demonstrate separate assignments to members of a union. */
#include <stdio.h>

typedef union {
  int number;
  char character;
} Value;

/**
 * @brief The main entry point of the program.
 * 
 * Demonstrates how a union works. Since all members in a union share the same 
 * memory space, assigning a value to one member might overwrite the value of 
 * another member. This code assigns and prints them separately.
 * 
 * @return 0 upon successful execution.
 */
int main(void) {
  Value value;

  value.number = 123;
  printf("Integer: %d\n", value.number);

  value.character = 'A';
  printf("Character: %c\n", value.character);

  return 0;
}