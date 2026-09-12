
/* Task 4 bonus: allocate and populate a variable-sized student collection. */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>


typedef struct {
  unsigned int id;
  char name[50];
  float marks;
} Student;

int main(void) {
  size_t n;

  printf("Enter number of students: ");

  if (scanf("%zu", &n) != 1 || n == 0) {
    printf("Invalid number of students\n");
    return 1;
  }

  /* Dynamically allocate students */
  Student *students = malloc(n * sizeof *students);

  if (students == NULL) {
    printf("Memory allocation failed\n");
    return 1;
  }

  /* Input student information */
  for (size_t i = 0; i < n; i++) {
    printf("\nStudent %zu\n", i + 1);

    printf("Enter ID: ");

    if (scanf("%u", &students[i].id) != 1)
      goto cleanup;

    printf("Enter name: ");

    if (scanf("%49s", students[i].name) != 1)
      goto cleanup;

    printf("Enter marks: ");

    if (scanf("%f", &students[i].marks) != 1)
      goto cleanup;
  }

  /* Display all students */
  printf("\n========== Students ==========\n");

  for (size_t i = 0; i < n; i++) {
    printf("\nStudent %zu\n", i + 1);
    printf("ID: %u\n", students[i].id);
    printf("Name: %s\n", students[i].name);
    printf("Marks: %.2f\n", students[i].marks);
  }

cleanup:
  free(students);

  return 0;
}