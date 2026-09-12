#include <stdlib.h>
#include <stdio.h>

int **transposeMatrix(int rows, int cols, const int matrix[rows][cols]) {
  int **transpose = malloc(cols * sizeof *transpose);

  if (transpose == NULL)
    return NULL;

  for (int i = 0; i < cols; i++) {
    transpose[i] = malloc(rows * sizeof *transpose[i]);

    if (transpose[i] == NULL) {
      for (int k = 0; k < i; k++)
        free(transpose[k]);

      free(transpose);
      return NULL;
    }
  }

  for (int i = 0; i < rows; i++)
    for (int j = 0; j < cols; j++)
      transpose[j][i] = matrix[i][j];

  return transpose;
}

int main(void) {
  int matrix[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
  int **transpose = transposeMatrix(3, 3, matrix);
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      printf("%d ", transpose[i][j]);
    }
    printf("\n");
  }
  return 0;
}