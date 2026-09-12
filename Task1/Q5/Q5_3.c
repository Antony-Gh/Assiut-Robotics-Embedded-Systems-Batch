/* Task 1, Q5.3: classify a positive year using the Gregorian leap-year rule. */
#include <stdio.h>

int main(void) {
  int year;

  printf("Enter a year: ");
  if (scanf("%d", &year) != 1 || year <= 0) {
    printf("Invalid year\n");
    return 1;
  }

  int k = 0;

  if ((year % 400 == 0) || (year % 4 == 0 && year % 100 != 0)) {
    k = 1;
  } else {
    k = 0;
  }

  switch (k) {
  case 1:
    printf("%d is a leap year\n", year);
    break;

  case 0:
    printf("%d is not a leap year\n", year);
    break;
  }

  return 0;
}