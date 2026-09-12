/* Task 3, Q5.1: demonstrate separate assignments to members of a union. */
#include <stdio.h>

typedef union {
  int number;
  char character;
} Value;

int main(void) {
  Value value;

  value.number = 123;
  printf("Integer: %d\n", value.number);

  value.character = 'A';
  printf("Character: %c\n", value.character);

  return 0;
}