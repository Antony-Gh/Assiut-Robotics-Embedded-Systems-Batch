#include <stdio.h>

void printMatrix(int rows, int cols, const int matrix[rows][cols]) {
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++)
      printf("%d ", matrix[i][j]);

    printf("\n");
  }
}

int matrixStats(int rows, int cols, const int matrix[rows][cols], int *min,
                int *max, double *average) {
  if (rows <= 0 || cols <= 0 || min == NULL || max == NULL || average == NULL)
    return 0;

  long long sum = 0;
  *min = matrix[0][0];
  *max = matrix[0][0];

  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      int value = matrix[i][j];

      if (value < *min)
        *min = value;

      if (value > *max)
        *max = value;

      sum += value;
    }
  }

  *average = (double)sum / (rows * cols);
  return 1;
}

int rowSums(int rows, int cols, const int matrix[rows][cols], long long sums[],
            long long *maxSum) {
  if (rows <= 0 || cols <= 0 || sums == NULL || maxSum == NULL)
    return 0;

  for (int i = 0; i < rows; i++) {
    sums[i] = 0;

    for (int j = 0; j < cols; j++)
      sums[i] += matrix[i][j];
  }

  *maxSum = sums[0];

  for (int i = 1; i < rows; i++)
    if (sums[i] > *maxSum)
      *maxSum = sums[i];

  return 1;
}

int main(void) {
  int rows, cols;

  printf("Enter rows and columns: ");
  if (scanf("%d %d", &rows, &cols) != 2 || rows <= 0 || cols <= 0)
    return 1;

  int matrix[rows][cols];

  for (int i = 0; i < rows; i++)
    for (int j = 0; j < cols; j++) {
      printf("matrix[%d][%d] = ", i, j);
      scanf("%d", &matrix[i][j]);
    }

  printf("Matrix:\n");
  printMatrix(rows, cols, matrix);

  int min, max;
  double average;

  matrixStats(rows, cols, matrix, &min, &max, &average);

  printf("Minimum: %d\nMaximum: %d\nAverage: %.2f\n", min, max, average);

  long long sums[rows];
  long long maxSum;

  rowSums(rows, cols, matrix, sums, &maxSum);
  printf("Maximum row sum: %lld\n", maxSum);

  return 0;
}