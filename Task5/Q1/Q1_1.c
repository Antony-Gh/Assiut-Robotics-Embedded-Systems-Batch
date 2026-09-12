
#include <stdio.h>

/**
 * @brief Computes the Greatest Common Divisor (GCD) of two integers.
 * 
 * Uses the Euclidean algorithm, repeatedly taking the remainder until one 
 * number becomes 0. Handles negative inputs by making them positive first.
 * 
 * @param a The first integer.
 * @param b The second integer.
 * @return The GCD of a and b.
 */
int gcd(int a, int b) {
  /* Convert negative values to nonnegative */
  if (a < 0)
    a = -a;

  if (b < 0)
    b = -b;

  while (b != 0) {
    int remainder = a % b;
    a = b;
    b = remainder;
  }

  return a;
}

/**
 * @brief The main entry point of the program.
 * 
 * Tests the gcd function with both positive and negative integers.
 * 
 * @return 0 upon successful execution.
 */
int main(void) {
  printf("GCD = %d\n", gcd(48, 18));
  printf("GCD = %d\n", gcd(-24, 18));

  return 0;
}