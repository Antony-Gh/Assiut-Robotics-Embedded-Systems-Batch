#include <ctype.h>
#include <stdio.h>
#include <string.h>


int isPalindrome(const char str[]) {
  size_t left = 0;
  size_t right = strlen(str);

  if (right == 0)
    return 1;

  right--;

  while (left < right) {
    while (left < right && !isalnum((unsigned char)str[left]))
      left++;

    while (left < right && !isalnum((unsigned char)str[right]))
      right--;

    if (tolower((unsigned char)str[left]) != tolower((unsigned char)str[right]))
      return 0;

    left++;
    right--;
  }

  return 1;
}

int main(void) {

  printf("%d\n", isPalindrome("anthony"));
  printf("%d\n", isPalindrome("gaius"));

  printf("%d\n", isPalindrome("madam"));
  printf("%d\n", isPalindrome("A man, a plan, a canal, Panama"));
  printf("%d\n", isPalindrome("Was it a car or a cat I saw?"));
  printf("%d\n", isPalindrome("12321"));
  return 0;
}