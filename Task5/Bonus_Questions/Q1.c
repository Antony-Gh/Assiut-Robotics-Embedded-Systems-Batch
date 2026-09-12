
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct {
  char name[50];
  int score;
} Student;

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

void sort_students_recursive(Student arr[], Student temp[], size_t left,
                             size_t right) {
  if (right - left <= 1)
    return;

  size_t mid = left + (right - left) / 2;

  sort_students_recursive(arr, temp, left, mid);
  sort_students_recursive(arr, temp, mid, right);

  merge_students(arr, temp, left, mid, right);
}

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