/* Task 1, Q5.3: classify a positive year using the Gregorian leap-year rule. */
#include <stdio.h>

/**
 * @brief The main entry point of the program.
 * 
 * This program determines if a given positive year is a leap year according to 
 * the Gregorian calendar rules. A year is a leap year if it is divisible by 400, 
 * or if it is divisible by 4 but NOT by 100.
 * 
 * @return 0 upon successful execution, 1 if the input is invalid or less than or equal to 0.
 */
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