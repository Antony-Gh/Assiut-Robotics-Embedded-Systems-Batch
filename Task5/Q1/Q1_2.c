
/* Task 5, Q1.2: compare nonnegative numeric strings and lexical order. */
#include <stdio.h>
#include <string.h>

/**
 * @brief Compares two strings as if they were nonnegative integers.
 * 
 * Compares string lengths first (longer string = larger number). 
 * If lengths are equal, falls back to lexicographical comparison (strcmp).
 * 
 * @param a The first string representing a number.
 * @param b The second string representing a number.
 * @return 1 if a > b, -1 if a < b, 0 if a == b.
 */
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

/**
 * @brief Finds both the numeric maximum and lexicographical maximum of two strings.
 * 
 * @param a The first string.
 * @param b The second string.
 * @param numeric_max Pointer to store the result of the numeric comparison.
 * @param lexicographic_max Pointer to store the result of the standard string comparison.
 */
void get_maximums(const char *a, const char *b, const char **numeric_max,
                  const char **lexicographic_max) {
  *numeric_max = numeric_compare(a, b) >= 0 ? a : b;
  *lexicographic_max = strcmp(a, b) >= 0 ? a : b;
}

/**
 * @brief The main entry point of the program.
 * 
 * Demonstrates the difference between numeric and lexicographical string comparisons.
 * (e.g., "12" > "4" numerically, but "12" < "4" lexicographically).
 * 
 * @return 0 upon successful execution.
 */
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