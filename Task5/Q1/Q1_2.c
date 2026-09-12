
#include <stdio.h>
#include <string.h>

int numeric_compare(const char *a, const char *b) {
  size_t len_a = strlen(a);
  size_t len_b = strlen(b);

  if (len_a > len_b)
    return 1;

  if (len_a < len_b)
    return -1;

  int result = strcmp(a, b);

  if (result > 0)
    return 1;

  if (result < 0)
    return -1;

  return 0;
}

void get_maximums(const char *a, const char *b, const char **numeric_max,
                  const char **lexicographic_max) {
  *numeric_max = numeric_compare(a, b) >= 0 ? a : b;
  *lexicographic_max = strcmp(a, b) >= 0 ? a : b;
}

int main(void) {
  const char *a = "12";
  const char *b = "4";

  const char *numeric_max;
  const char *lexicographic_max;

  get_maximums(a, b, &numeric_max, &lexicographic_max);

  printf("Numeric maximum: %s\n", numeric_max);
  printf("Lexicographic maximum: %s\n", lexicographic_max);

  return 0;
}