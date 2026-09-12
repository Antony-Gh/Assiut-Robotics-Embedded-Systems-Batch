/* Task 2, Q5.5: test palindromes while ignoring case and punctuation. */
#include <ctype.h>
#include <stdio.h>
#include <string.h>


/**
 * @brief Checks if a string is a palindrome.
 * 
 * A palindrome reads the same forwards and backwards. This function ignores
 * case differences and non-alphanumeric characters (like punctuation and spaces).
 * 
 * @param str The string to check.
 * @return 1 if the string is a palindrome, 0 otherwise.
 */
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

/**
 * @brief The main entry point of the program.
 * 
 * Tests various strings to see if they are palindromes and prints the result
 * (1 for true, 0 for false).
 * 
 * @return 0 upon successful execution.
 */
int main(void) {

  printf("%d\n", isPalindrome("anthony"));
  printf("%d\n", isPalindrome("gaius"));

  printf("%d\n", isPalindrome("madam"));
  printf("%d\n", isPalindrome("A man, a plan, a canal, Panama"));
  printf("%d\n", isPalindrome("Was it a car or a cat I saw?"));
  printf("%d\n", isPalindrome("12321"));
  return 0;
}