
#include <stdio.h>

int isSymmetric(int rows, int cols, const int matrix[rows][cols]) {
  if (rows != cols)
    return 0;

  for (int i = 0; i < rows; i++)
    for (int j = i + 1; j < cols; j++)
      if (matrix[i][j] != matrix[j][i])
        return 0;

  return 1;
}

int main(void) {
  int matrix[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
  if (isSymmetric(3, 3, matrix)) {
    printf("The matrix is symmetric.\n");
  } else {
    printf("The matrix is not symmetric.\n");
  }
  return 0;
}