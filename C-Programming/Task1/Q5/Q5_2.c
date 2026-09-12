/* Task 1, Q5.2: perform basic arithmetic selected by an operator. */
#include <stdio.h>

/**
 * @brief The main entry point of the program.
 * 
 * This program acts as a simple calculator. It asks the user to input an arithmetic 
 * operator (+, -, *, /) and two numbers. It then uses a switch statement to perform
 * the requested operation and displays the result. It also handles division by zero.
 * 
 * @return 0 upon successful execution, 1 if input parsing fails.
 */
int main(void) {
  double a, b;
  char op;

  printf("Enter an operator (+, -, *, /): ");
  if (scanf(" %c", &op) != 1)
    return 1;

  printf("Enter two numbers: ");
  if (scanf("%lf %lf", &a, &b) != 2)
    return 1;

  switch (op) {
  case '+':
    printf("Result: %.2f\n", a + b);
    break;

  case '-':
    printf("Result: %.2f\n", a - b);
    break;

  case '*':
    printf("Result: %.2f\n", a * b);
    break;

  case '/':
    if (b == 0.0)
      printf("Error: division by zero\n");
    else
      printf("Result: %.2f\n", a / b);
    break;

  default:
    printf("Invalid operator\n");
  }

  return 0;
}