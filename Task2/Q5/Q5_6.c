#include <ctype.h>

#include <stdio.h>

void replaceWhitespace(char str[]) {
  for (int i = 0; str[i] != '\0'; i++) {
    if (isspace((unsigned char)str[i]))
      str[i] = '_';
  }
}

int main(void) {
  char str[] = "Hello, World!";
  replaceWhitespace(str);
  printf("%s\n", str);

  char str2[] = "This is a test string!";
  replaceWhitespace(str2);
  printf("%s\n", str2);

  return 0;
}
