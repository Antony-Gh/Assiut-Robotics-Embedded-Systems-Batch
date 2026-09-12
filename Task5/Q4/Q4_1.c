
/* Task 5, Q4.1: identify a number with three-way range questions. */
#include <stdio.h>

/**
 * @brief The main entry point of the program.
 * 
 * Plays a more advanced number guessing game using ternary-like search intervals.
 * It divides the search space into three parts and asks the user which part 
 * contains their number (below, inside, or above the middle range).
 * 
 * @return 0 upon successful execution, 1 on invalid input.
 */
int main(void) {
  int low = 1;
  int high = 1000;
  int answer;

  printf("Think of a number from 1 to 1000.\n");
  printf("Answer 0 = below, 1 = inside, 2 = above.\n");

  while (low < high) {
    int length = high - low + 1;

    int first;
    int second;

    if (length == 2) {
      first = low;
      second = low;
    } else {
      int third = length / 3;
      first = low + third;
      second = high - third;
    }

    printf("Is your number in [%d, %d]?\n", first, second);
    printf("Enter 0, 1, or 2: ");

    if (scanf("%d", &answer) != 1)
      return 1;

    if (length == 2) {
      if (answer == 1)
        high = low;
      else if (answer == 2)
        low = high;
      else
        printf("Invalid answer for this range.\n");
    } else if (answer == 0) {
      high = first - 1;
    } else if (answer == 1) {
      low = first;
      high = second;
    } else if (answer == 2) {
      low = second + 1;
    } else {
      printf("Invalid answer.\n");
    }
  }

  printf("Your number is %d!\n", low);

  return 0;
}