
/* Task 5 bonus: stable-merge-sort students by name and then score. */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct {
  char name[50];
  int score;
} Student;

/**
 * @brief Compares two students first by name, then by score.
 * 
 * This is used to maintain a stable sorting order.
 * 
 * @param a Pointer to the first Student.
 * @param b Pointer to the second Student.
 * @return A negative number if a < b, positive if a > b, 0 if equal.
 */
int compare_students(const Student *a, const Student *b) {
  int name_result = strcmp(a->name, b->name);

  if (name_result != 0)
    return name_result;

  if (a->score < b->score)
    return -1;

  if (a->score > b->score)
    return 1;

  return 0;
}

/**
 * @brief Merges two sorted halves of a Student array into a temporary array.
 * 
 * @param arr The original array containing the halves.
 * @param temp The temporary array used for merging.
 * @param left The starting index of the left half.
 * @param mid The starting index of the right half.
 * @param right The ending index (exclusive) of the right half.
 */
void merge_students(Student arr[], Student temp[], size_t left, size_t mid,
                    size_t right) {
  size_t i = left;
  size_t j = mid;
  size_t k = left;

  while (i < mid && j < right) {
    if (compare_students(&arr[i], &arr[j]) <= 0)
      temp[k++] = arr[i++];
    else
      temp[k++] = arr[j++];
  }

  while (i < mid)
    temp[k++] = arr[i++];

  while (j < right)
    temp[k++] = arr[j++];

  for (size_t p = left; p < right; p++)
    arr[p] = temp[p];
}

/**
 * @brief Recursively splits the Student array for Merge Sort.
 * 
 * @param arr The array to sort.
 * @param temp A temporary array used for merging.
 * @param left The starting index of the range to sort.
 * @param right The ending index (exclusive) of the range.
 */
void sort_students_recursive(Student arr[], Student temp[], size_t left,
                             size_t right) {
  if (right - left <= 1)
    return;

  size_t mid = left + (right - left) / 2;

  sort_students_recursive(arr, temp, left, mid);
  sort_students_recursive(arr, temp, mid, right);

  merge_students(arr, temp, left, mid, right);
}

/**
 * @brief Sorts an array of Students using the Merge Sort algorithm.
 * 
 * Memory for a temporary array is allocated dynamically to aid the merge process.
 * 
 * @param arr The array of Students to sort.
 * @param n The total number of elements.
 * @return 1 on success, 0 on failure (e.g., memory allocation error).
 */
int sort_students(Student arr[], size_t n) {
  if (n < 2)
    return 1;

  if (n > (size_t)-1 / sizeof(Student))
    return 0;

  Student *temp = malloc(n * sizeof *temp);

  if (temp == NULL)
    return 0;

  sort_students_recursive(arr, temp, 0, n);

  free(temp);
  return 1;
}

/**
 * @brief The main entry point of the program.
 * 
 * Tests the merge sort implementation on an array of Students.
 * 
 * @return 0 upon successful execution.
 */
int main(void) {
  Student students[] = {
      {"Sara", 90}, {"Ahmed", 85}, {"Sara", 75}, {"Omar", 80}, {"Ahmed", 95}};

  size_t n = sizeof(students) / sizeof(students[0]);

  if (!sort_students(students, n))
    return 1;

  for (size_t i = 0; i < n; i++)
    printf("%s - %d\n", students[i].name, students[i].score);

  return 0;
}