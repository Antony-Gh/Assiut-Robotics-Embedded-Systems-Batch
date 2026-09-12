
#include <stdio.h>

int main(void)
{
    int low = 1;
    int high = 1000;
    char answer;

    printf("Think of a number from 1 to 1000.\n");

    while (low < high)
    {
        int mid = low + (high - low) / 2;

        printf("Is your number greater than %d? (Y/N): ", mid);

        if (scanf(" %c", &answer) != 1)
            return 1;

        if (answer == 'Y' || answer == 'y')
            low = mid + 1;
        else if (answer == 'N' || answer == 'n')
            high = mid;
        else
        {
            printf("Invalid answer. Enter Y or N.\n");
            continue;
        }
    }

    printf("Your number is %d!\n", low);

    return 0;
}