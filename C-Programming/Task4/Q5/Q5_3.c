
/* Task 4, Q5.3: allocate, populate, and release a jagged integer matrix. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


/**
 * @brief The main entry point of the program.
 * 
 * Demonstrates allocating a "jagged" 2D array, where each row can have a 
 * different number of columns. Prompts the user for dimensions and data, 
 * prints the matrix, and properly frees the dynamically allocated memory.
 * 
 * @return 0 upon successful execution, 1 on error.
 */
int main(void) {
  size_t rows;

  printf("Enter number of rows: ");

  if (scanf("%zu", &rows) != 1 || rows == 0)
    return 1;

  if (rows > SIZE_MAX / sizeof(int *))
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

    if (cols[i] > SIZE_MAX / sizeof *matrix[i])
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