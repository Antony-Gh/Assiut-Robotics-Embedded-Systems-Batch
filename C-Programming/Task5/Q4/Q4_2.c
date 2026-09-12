
/* Task 5, Q4.2: stable-merge-sort points by squared reference distance. */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>


typedef struct {
  double x;
  double y;
} Point;

/**
 * @brief Calculates the squared distance between a point and an origin.
 * 
 * Uses the formula: dx^2 + dy^2. We use squared distance to avoid expensive 
 * square root calculations since we only need it for comparison.
 * 
 * @param p The point to calculate the distance for.
 * @param origin The reference origin point.
 * @return The squared distance as a double.
 */
double squared_distance(Point p, Point origin) {
  double dx = p.x - origin.x;
  double dy = p.y - origin.y;

  return dx * dx + dy * dy;
}

/**
 * @brief Merges two sorted sub-arrays into a single sorted array based on distance.
 * 
 * @param arr The original array containing the two sub-arrays.
 * @param temp A temporary buffer array used for merging.
 * @param left The starting index of the left sub-array.
 * @param mid The ending index of the left sub-array (and start of the right).
 * @param right The ending index of the right sub-array.
 * @param origin The reference point used for calculating distances.
 */
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

/**
 * @brief Recursively divides the array and sorts it using merge sort.
 * 
 * @param arr The array to sort.
 * @param temp A temporary array used for merging.
 * @param left The starting index of the sub-array.
 * @param right The ending index of the sub-array.
 * @param origin The reference point used for calculating distances.
 */
void merge_sort_recursive(Point arr[], Point temp[], size_t left, size_t right,
                          Point origin) {
  if (right - left <= 1)
    return;

  size_t mid = left + (right - left) / 2;

  merge_sort_recursive(arr, temp, left, mid, origin);
  merge_sort_recursive(arr, temp, mid, right, origin);

  merge(arr, temp, left, mid, right, origin);
}

/**
 * @brief Sorts an array of points based on their distance from an origin.
 * 
 * This is the public entry point for sorting. It handles memory allocation 
 * for the temporary buffer needed by merge sort.
 * 
 * @param arr The array of points to sort.
 * @param n The number of points in the array.
 * @param origin The reference point used for calculating distances.
 * @return 1 on success, 0 on memory allocation failure or invalid size.
 */
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

/**
 * @brief The main entry point of the program.
 * 
 * Defines an array of points and an origin, sorts the points based on their 
 * distance to the origin using merge sort, and prints the result.
 * 
 * @return 0 upon successful execution, 1 on error.
 */
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