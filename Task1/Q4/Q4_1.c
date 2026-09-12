/* Task 1, Q4: read integers until stopped and report min, max, and average. */
#include <stdio.h>

int main(void) {
  int number;
  int min, max;
  int count = 0;
  long double sum = 0.0L;
  char choice;

  do {
    printf("Enter an integer: ");

    if (scanf("%d", &number) != 1) {
      printf("Invalid input.\n");
      return 1;
    }

    if (count == 0) {
      min = max = number;
    } else {
      if (number < min)
        min = number;

      if (number > max)
        max = number;
    }

    sum += number;
    count++;

    printf("Continue? (y/n): ");
    if (scanf(" %c", &choice) != 1)
      return 1;

  } while (choice != 'n' && choice != 'N');

  printf("\nMinimum: %d\n", min);
  printf("Maximum: %d\n", max);
  printf("Average: %.2Lf\n", sum / count);

  return 0;
}