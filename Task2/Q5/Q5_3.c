#include <stdio.h>

int concatenate(char dest[], size_t capacity, const char src[]) {
  size_t i = 0;
  size_t j = 0;

  while (i < capacity && dest[i] != '\0')
    i++;

  if (i == capacity)
    return 0; // dest was not terminated within capacity

  while (src[j] != '\0') {
    if (i + 1 >= capacity)
      return 0; // Not enough space

    dest[i++] = src[j++];
  }

  dest[i] = '\0';
  return 1;
}

int main(void) {
  char first[30] = "Hello ";
  char second[] = "World";

  if (concatenate(first, sizeof first, second))
    printf("%s\n", first);
}
