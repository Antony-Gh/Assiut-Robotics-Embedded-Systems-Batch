
/* Task 4, Q5.2: copy a string into independently allocated storage. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Creates a dynamically allocated copy of a string.
 * 
 * Calculates the length of the source string, allocates enough memory to hold 
 * the characters plus the null terminator, and copies the contents over.
 * 
 * @param source The null-terminated source string to copy.
 * @return A pointer to the newly allocated string copy, or NULL on failure.
 */
char *copy_string(const char *source) {
  if (source == NULL)
    return NULL;

  size_t length = strlen(source);

  char *copy = malloc(length + 1);

  if (copy == NULL)
    return NULL;

  memcpy(copy, source, length + 1);

  return copy;
}

/**
 * @brief The main entry point of the program.
 * 
 * Demonstrates copying a string into an independent memory location using 
 * the copy_string function, prints both strings, and then frees the copy.
 * 
 * @return 0 upon successful execution, 1 on allocation failure.
 */
int main(void) {
  const char *original = "Hello C17";

  char *copy = copy_string(original);

  if (copy == NULL) {
    printf("Memory allocation failed\n");
    return 1;
  }

  printf("Original: %s\n", original);
  printf("Copy: %s\n", copy);

  free(copy);

  return 0;
}