/* Task 3, Q4.2: read employee records and validate role enum values. */
#include <stdio.h>
#include <stdlib.h>

typedef enum { ENGINEER, MANAGER, HR } Role;

typedef struct {
  char name[50];
  Role role;
} Employee;

static const char *role_name(Role role) {
  switch (role) {
  case ENGINEER:
    return "Engineer";
  case MANAGER:
    return "Manager";
  case HR:
    return "HR";
  default:
    return "Unknown";
  }
}

int main(void) {
  size_t count;

  printf("Number of employees: ");
  if (scanf("%zu", &count) != 1 || count == 0)
    return 1;

  if (count > (size_t)-1 / sizeof(Employee))
    return 1;

  Employee *employees = malloc(count * sizeof *employees);
  if (employees == NULL) {
    fprintf(stderr, "Memory allocation failed.\n");
    return 1;
  }

  for (size_t i = 0; i < count; ++i) {
    int role;

    printf("Employee %zu name (one word): ", i + 1);
    if (scanf("%49s", employees[i].name) != 1)
      goto input_error;

    printf("Role (0=Engineer, 1=Manager, 2=HR): ");
    if (scanf("%d", &role) != 1 || role < ENGINEER || role > HR)
      goto input_error;

    employees[i].role = (Role)role;
  }

  puts("\nEmployee details:");
  for (size_t i = 0; i < count; ++i) {
    printf("Name: %s, Role: %s\n", employees[i].name,
           role_name(employees[i].role));
  }

  free(employees);
  return 0;

input_error:
  fprintf(stderr, "Invalid input.\n");
  free(employees);
  return 1;
}