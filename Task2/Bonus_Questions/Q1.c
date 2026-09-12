#include <stdio.h>
#include <string.h>

void splitString(char str[], const char *delimiters)
{
    char *token = strtok(str, delimiters);

    while (token != NULL)
    {
        printf("%s\n", token);
        token = strtok(NULL, delimiters);
    }
}

int main(void)
{
    char text[] = "red,green,blue";
    splitString(text, ",");
    return 0;
}