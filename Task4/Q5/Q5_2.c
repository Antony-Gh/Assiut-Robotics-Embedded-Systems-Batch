
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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