
/* Task 4, Q4.2: read a student record using shared integer typedefs. */
#include <inttypes.h>
#include <stdio.h>
#include "STD_TYPES.h"

typedef struct
{
    u32 id;
    char name[50];
    float marks;
} Student;

/**
 * @brief The main entry point of the program.
 * 
 * Demonstrates reading and storing data into a structure using shared 
 * standard integer types (like u32) and formats macros (like SCNu32, PRIu32) 
 * for safe input and output.
 * 
 * @return 0 upon successful execution, 1 on invalid input.
 */
int main(void)
{
    Student s;

    printf("Enter student ID: ");
    if (scanf("%u", &s.id) != 1)
        return 1;

    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    printf("Enter student name: ");
    if (scanf("%49s", s.name) != 1)
        return 1;

    printf("Enter student marks: ");
    if (scanf("%f", &s.marks) != 1)
        return 1;

    printf("\nStudent Information\n");
    printf("ID: %u\n", s.id);
    printf("Name: %s\n", s.name);
    printf("Marks: %.2f\n", s.marks);

    return 0;
}