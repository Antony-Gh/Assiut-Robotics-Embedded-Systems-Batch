
#include <stdio.h>

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

int main(void) {

    char str[] = "Hello, World!";
    reverseString(str);
    printf("Reversed string: %s\n", str);

    char str2[] = "Anthony Gaius";
    reverseString(str2);
    printf("Reversed string: %s\n", str2);


    return 0;
}