
#include <stdio.h>

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

int main(void) {
  printf("IS_PRIME(2)  = %d\n", IS_PRIME(2));
  printf("IS_PRIME(7)  = %d\n", IS_PRIME(7));
  printf("IS_PRIME(9)  = %d\n", IS_PRIME(9));
  printf("IS_PRIME(25) = %d\n", IS_PRIME(25));
  printf("IS_PRIME(1)  = %d\n", IS_PRIME(1));
  printf("IS_PRIME(-7) = %d\n", IS_PRIME(-7));

  return 0;
}