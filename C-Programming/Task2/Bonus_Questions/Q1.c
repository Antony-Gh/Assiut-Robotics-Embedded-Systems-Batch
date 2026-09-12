/* Task 2 bonus: split a mutable string into delimiter-separated tokens. */
#include <stdio.h>
#include <string.h>

/**
 * @brief Splits a string into tokens based on specified delimiters.
 * 
 * Uses the strtok function to continuously break down a string. 
 * Note that strtok modifies the original string by inserting null terminators.
 * 
 * @param str The string to split.
 * @param delimiters A string containing all delimiter characters.
 */
void splitString(char str[], const char *delimiters)
{
    char *token = strtok(str, delimiters);

    while (token != NULL)
    {
        printf("%s\n", token);
        token = strtok(NULL, delimiters);
    }
}

/**
 * @brief The main entry point of the program.
 * 
 * Demonstrates splitting a comma-separated string into individual words.
 * 
 * @return 0 upon successful execution.
 */
int main(void)
{
    char text[] = "red,green,blue";
    splitString(text, ",");
    return 0;
}