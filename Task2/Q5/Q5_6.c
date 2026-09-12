/* Task 2, Q5.6: replace every whitespace character with an underscore. */
#include <ctype.h>

#include <stdio.h>

/**
 * @brief Replaces all whitespace characters in a string with underscores.
 * 
 * Modifies the string in place.
 * 
 * @param str The string to modify.
 */
void replaceWhitespace(char str[]) {
  for (int i = 0; str[i] != '\0'; i++) {
    if (isspace((unsigned char)str[i]))
      str[i] = '_';
  }
}

/**
 * @brief The main entry point of the program.
 * 
 * Demonstrates the replaceWhitespace function on sample strings.
 * 
 * @return 0 upon successful execution.
 */
int main(void) {
  char str[] = "Hello, World!";
  replaceWhitespace(str);
  printf("%s\n", str);

  char str2[] = "This is a test string!";
  replaceWhitespace(str2);
  printf("%s\n", str2);

  return 0;
}
