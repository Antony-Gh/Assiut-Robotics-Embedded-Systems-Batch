/* Task 3, Q2: allocate student records and sort by total marks then name. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char name[20];
  int arabic;
  int english;
  int math;
} Student;

/**
 * @brief Calculates the total marks of a student.
 * 
 * @param s Pointer to the Student struct.
 * @return The sum of Arabic, English, and Math marks.
 */
static int total_marks(const Student *s) {
  return s->arabic + s->english + s->math;
}

/* Returns nonzero if a should appear before b. */
/**
 * @brief Compares two students for sorting.
 * 
 * A student comes before another if they have a higher total mark.
 * If total marks are equal, they are sorted alphabetically by name.
 * 
 * @param a Pointer to the first student.
 * @param b Pointer to the second student.
 * @return 1 (nonzero) if student 'a' should appear before student 'b', 0 otherwise.
 */
static int comes_before(const Student *a, const Student *b) {
  int total_a = total_marks(a);
  int total_b = total_marks(b);

  if (total_a != total_b)
    return total_a > total_b;

  return strcmp(a->name, b->name) < 0;
}

/**
 * @brief Sorts an array of students using Bubble Sort.
 * 
 * Sorts students based on their total marks (descending) and then by name 
 * (alphabetical) using the comes_before function.
 * 
 * @param students The array of Student structs to sort.
 * @param count The number of students in the array.
 */
static void sort_students(Student students[], size_t count) {
  for (size_t i = 0; i < count; ++i) {
    for (size_t j = 0; j + 1 < count - i; ++j) {
      if (!comes_before(&students[j], &students[j + 1])) {
        Student temp = students[j];
        students[j] = students[j + 1];
        students[j + 1] = temp;
      }
    }
  }
}

/**
 * @brief The main entry point of the program.
 * 
 * Prompts the user for the number of students, dynamically allocates memory 
 * for them, reads their details, sorts them, and then prints the sorted list.
 * 
 * @return 0 upon successful execution, 1 on error.
 */
int main(void) {
  size_t count;

  printf("Number of students: ");
  if (scanf("%zu", &count) != 1 || count == 0)
    return 1;

  if (count > (size_t)-1 / sizeof(Student))
    return 1;

  Student *students = malloc(count * sizeof *students);
  if (students == NULL) {
    fprintf(stderr, "Memory allocation failed.\n");
    return 1;
  }

  for (size_t i = 0; i < count; ++i) {
    printf("Student %zu name (max 19 characters): ", i + 1);
    if (scanf("%19s", students[i].name) != 1)
      goto input_error;

    printf("Arabic English Math marks: ");
    if (scanf("%d %d %d", &students[i].arabic, &students[i].english,
              &students[i].math) != 3)
      goto input_error;
  }

  sort_students(students, count);

  puts("\nSorted students:");
  for (size_t i = 0; i < count; ++i) {
    printf("%s — Arabic: %d, English: %d, Math: %d, Total: %d\n",
           students[i].name, students[i].arabic, students[i].english,
           students[i].math, total_marks(&students[i]));
  }

  free(students);
  return 0;

input_error:
  fprintf(stderr, "Invalid input.\n");
  free(students);
  return 1;
}