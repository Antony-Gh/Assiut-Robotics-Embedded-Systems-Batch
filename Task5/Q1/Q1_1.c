
#include <stdio.h>

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

int main(void) {
  printf("GCD = %d\n", gcd(48, 18));
  printf("GCD = %d\n", gcd(-24, 18));

  return 0;
}