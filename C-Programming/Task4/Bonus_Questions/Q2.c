
/* Task 4 bonus: check primality and expose the result through a macro. */
#include <stdio.h>

/**
 * @brief Checks if a number is a prime number.
 * 
 * A prime number is a positive integer greater than 1 that has no positive 
 * divisors other than 1 and itself. This function checks for divisibility 
 * up to the square root of the number for efficiency.
 * 
 * @param n The integer to check.
 * @return 1 if the number is prime, 0 otherwise.
 */
static int is_prime(int n) {
  if (n < 2)
    return 0;

  if (n == 2)
    return 1;

  if (n % 2 == 0)
    return 0;

  for (int i = 3; i <= n / i; i += 2) {
    if (n % i == 0)
      return 0;
  }

  return 1;
}

#define IS_PRIME(n) is_prime((n))

/**
 * @brief The main entry point of the program.
 * 
 * Tests the IS_PRIME macro on a series of test integers.
 * 
 * @return 0 upon successful execution.
 */
int main(void) {
  printf("IS_PRIME(2)  = %d\n", IS_PRIME(2));
  printf("IS_PRIME(7)  = %d\n", IS_PRIME(7));
  printf("IS_PRIME(9)  = %d\n", IS_PRIME(9));
  printf("IS_PRIME(25) = %d\n", IS_PRIME(25));
  printf("IS_PRIME(1)  = %d\n", IS_PRIME(1));
  printf("IS_PRIME(-7) = %d\n", IS_PRIME(-7));

  return 0;
}