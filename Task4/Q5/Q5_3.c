
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>


int main(void) {
  size_t rows;

  printf("Enter number of rows: ");

  if (scanf("%zu", &rows) != 1 || rows == 0)
    return 1;

  int **matrix = calloc(rows, sizeof *matrix);
  size_t *cols = calloc(rows, sizeof *cols);

  if (matrix == NULL || cols == NULL) {
    free(matrix);
    free(cols);
    return 1;
  }

  for (size_t i = 0; i < rows; i++) {
    printf("Columns in row %zu: ", i + 1);

    if (scanf("%zu", &cols[i]) != 1 || cols[i] == 0)
      goto cleanup;

    matrix[i] = malloc(cols[i] * sizeof *matrix[i]);

    if (matrix[i] == NULL)
      goto cleanup;

    for (size_t j = 0; j < cols[i]; j++) {
      printf("matrix[%zu][%zu] = ", i, j);

      if (scanf("%d", &matrix[i][j]) != 1)
        goto cleanup;
    }
  }

  printf("\nMatrix:\n");

  for (size_t i = 0; i < rows; i++) {
    for (size_t j = 0; j < cols[i]; j++)
      printf("%d ", matrix[i][j]);

    printf("\n");
  }

cleanup:
  for (size_t i = 0; i < rows; i++)
    free(matrix[i]);

  free(matrix);
  free(cols);

  return 0;
}