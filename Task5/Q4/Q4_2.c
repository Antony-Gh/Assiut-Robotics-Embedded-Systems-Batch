
/* Task 5, Q4.2: stable-merge-sort points by squared reference distance. */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>


typedef struct {
  double x;
  double y;
} Point;

double squared_distance(Point p, Point origin) {
  double dx = p.x - origin.x;
  double dy = p.y - origin.y;

  return dx * dx + dy * dy;
}

void merge(Point arr[], Point temp[], size_t left, size_t mid, size_t right,
           Point origin) {
  size_t i = left;
  size_t j = mid;
  size_t k = left;

  while (i < mid && j < right) {
    if (squared_distance(arr[i], origin) <= squared_distance(arr[j], origin)) {
      temp[k++] = arr[i++];
    } else {
      temp[k++] = arr[j++];
    }
  }

  while (i < mid)
    temp[k++] = arr[i++];

  while (j < right)
    temp[k++] = arr[j++];

  for (size_t p = left; p < right; p++)
    arr[p] = temp[p];
}

void merge_sort_recursive(Point arr[], Point temp[], size_t left, size_t right,
                          Point origin) {
  if (right - left <= 1)
    return;

  size_t mid = left + (right - left) / 2;

  merge_sort_recursive(arr, temp, left, mid, origin);
  merge_sort_recursive(arr, temp, mid, right, origin);

  merge(arr, temp, left, mid, right, origin);
}

int sort_points(Point arr[], size_t n, Point origin) {
  if (n < 2)
    return 1;

  if (n > (size_t)-1 / sizeof(Point))
    return 0;

  Point *temp = malloc(n * sizeof *temp);

  if (temp == NULL)
    return 0;

  merge_sort_recursive(arr, temp, 0, n, origin);

  free(temp);
  return 1;
}

int main(void) {
  Point points[] = {{3, 4}, {1, 1}, {0, 2}, {5, 5}};

  size_t n = sizeof(points) / sizeof(points[0]);
  Point origin = {0, 0};

  if (!sort_points(points, n, origin)) {
    printf("Memory allocation failed\n");
    return 1;
  }

  printf("Points sorted by distance:\n");

  for (size_t i = 0; i < n; i++) {
    printf("(%.1f, %.1f) Distance squared = %.1f\n", points[i].x, points[i].y,
           squared_distance(points[i], origin));
  }

  return 0;
}