
#include <stdio.h>

void copyString(char dest[], const char src[]) {
  int i = 0;

  do {
    dest[i] = src[i];
  } while (src[i++] != '\0');
}

int main(void) {
  char first[30] = "Hello ";
  char second[] = "World";

  printf("%s\n", first);
  printf("%s\n", second);

  copyString(first, second);

  printf("%s\n", first);
  printf("%s\n", second);

  return 0;
}