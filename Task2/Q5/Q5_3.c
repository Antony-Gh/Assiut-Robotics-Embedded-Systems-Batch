/* Task 2, Q5.3: concatenate strings while respecting destination capacity. */
#include <stdio.h>

/**
 * @brief Concatenates a source string to a destination string safely.
 * 
 * Appends the characters of the source string to the end of the destination
 * string, ensuring that it doesn't exceed the provided capacity.
 * 
 * @param dest The destination string buffer.
 * @param capacity The maximum size of the destination buffer.
 * @param src The source string to append.
 * @return 1 on success, 0 if there wasn't enough space or dest was invalid.
 */
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

/**
 * @brief The main entry point of the program.
 * 
 * Demonstrates the use of the concatenate function to join two strings.
 * 
 * @return 0 upon successful execution.
 */
int main(void) {
  char first[30] = "Hello ";
  char second[] = "World";

  if (concatenate(first, sizeof first, second))
    printf("%s\n", first);
}
