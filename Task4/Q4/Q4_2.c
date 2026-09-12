
#include <stdio.h>
#include "STD_TYPES.h"

typedef struct
{
    u32 id;
    char name[50];
    float marks;
} Student;

int main(void)
{
    Student s;

    printf("Enter student ID: ");
    scanf("%u", &s.id);

    printf("Enter student name: ");
    scanf("%49s", s.name);

    printf("Enter student marks: ");
    scanf("%f", &s.marks);

    printf("\nStudent Information\n");
    printf("ID: %u\n", s.id);
    printf("Name: %s\n", s.name);
    printf("Marks: %.2f\n", s.marks);

    return 0;
}