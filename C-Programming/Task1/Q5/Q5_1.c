/* Task 1, Q5.1: map a numeric day selection to a weekday with switch. */
#include <stdio.h>

/**
 * @brief The main entry point of the program.
 * 
 * This program asks the user to input a day number (from 1 to 7).
 * It uses a switch statement to map the numeric input to the corresponding
 * weekday name (e.g., 1 -> Monday) and prints it to the console.
 * 
 * @return 0 upon successful execution, 1 if the input is not a valid number.
 */
int main(void)
{
    int day;

    printf("Enter a day number (1-7): ");
    if (scanf("%d", &day) != 1)
        return 1;

    switch (day)
    {
        case 1: printf("Monday\n");    break;
        case 2: printf("Tuesday\n");   break;
        case 3: printf("Wednesday\n"); break;
        case 4: printf("Thursday\n");  break;
        case 5: printf("Friday\n");    break;
        case 6: printf("Saturday\n");  break;
        case 7: printf("Sunday\n");    break;
        default: printf("Invalid day number\n");
    }

    return 0;
}