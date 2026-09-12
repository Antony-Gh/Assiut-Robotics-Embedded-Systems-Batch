/* Task 2, Q1.2: reverse a null-terminated string in place with pointers. */
#include <stdio.h>

/**
 * @brief Reverses a string in place using pointers.
 * 
 * Uses two pointers (left at the start, right at the end) and swaps the
 * characters they point to, moving them towards the center.
 * 
 * @param str The null-terminated string to reverse.
 */
void reverseString(char *str)
{
    if (str == NULL)
        return;

    char *left = str;
    char *right = str;

    while (*right != '\0')
        right++;

    if (right == str)
        return;

    right--;  // Point to the last character

    while (left < right)
    {
        char temp = *left;
        *left = *right;
        *right = temp;

        left++;
        right--;
    }
}

/**
 * @brief The main entry point of the program.
 * 
 * Demonstrates reversing sample strings.
 * 
 * @return 0 upon successful execution.
 */
int main(void) {

    char str[] = "Hello, World!";
    reverseString(str);
    printf("Reversed string: %s\n", str);

    char str2[] = "Anthony Gaius";
    reverseString(str2);
    printf("Reversed string: %s\n", str2);


    return 0;
}