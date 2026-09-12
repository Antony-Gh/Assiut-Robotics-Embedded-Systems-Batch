
/* Task 5, Q3.3: identify a number from 1 through 1000 with binary questions. */
#include <stdio.h>

/**
 * @brief The main entry point of the program.
 * 
 * Plays a number guessing game. It asks the user to think of a number between 
 * 1 and 1000, and uses binary search to guess it by repeatedly asking if the 
 * number is greater than the current midpoint.
 * 
 * @return 0 upon successful execution, 1 on invalid input.
 */
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