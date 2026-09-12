
/* Copy a null-terminated string; the caller must provide enough destination space. */
/* Copy a null-terminated string; the caller must provide enough destination space. */
#include <stdio.h>

/**
 * @brief Copies a string from the source to the destination.
 * 
 * Note: The caller must ensure that the destination buffer is large enough
 * to hold the source string, including the null terminator.
 * 
 * @param dest The destination buffer.
 * @param src The source string to copy.
 */
void copyString(char dest[], const char src[]) {
  int i = 0;

  do {
    dest[i] = src[i];
  } while (src[i++] != '\0');
}

/**
 * @brief The main entry point of the program.
 * 
 * Demonstrates copying a string using the copyString function.
 * 
 * @return 0 upon successful execution.
 */
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