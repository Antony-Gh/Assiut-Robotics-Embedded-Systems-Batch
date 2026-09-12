#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

/* Allocate and return the transpose; the caller owns the returned rows. */
/**
 * @brief Creates a dynamically allocated transpose of a given 2D matrix.
 * 
 * The transpose of a matrix swaps its rows and columns. This function allocates
 * memory for the new matrix. The caller is responsible for freeing this memory.
 * 
 * @param rows The number of rows in the original matrix.
 * @param cols The number of columns in the original matrix.
 * @param matrix The original 2D array.
 * @return A pointer to the newly allocated transposed matrix, or NULL on failure.
 */
int **transposeMatrix(int rows, int cols, const int matrix[rows][cols]) {
  if (rows <= 0 || cols <= 0 || (size_t)cols > SIZE_MAX / sizeof(int *))
    return NULL;
  int **transpose = malloc(cols * sizeof *transpose);

  if (transpose == NULL)
    return NULL;

  int allocated = 0;

  for (int i = 0; i < cols; i++) {
    if ((size_t)rows > SIZE_MAX / sizeof(int))
      goto allocation_failure;

    transpose[i] = malloc(rows * sizeof *transpose[i]);

    if (transpose[i] == NULL)
      goto allocation_failure;

    allocated++;
  }

  for (int i = 0; i < rows; i++)
    for (int j = 0; j < cols; j++)
      transpose[j][i] = matrix[i][j];

  return transpose;

allocation_failure:
  for (int k = 0; k < allocated; k++)
    free(transpose[k]);

  free(transpose);
  return NULL;
}

/**
 * @brief The main entry point of the program.
 * 
 * Demonstrates transposing a 3x3 matrix, prints the result, and correctly
 * frees the dynamically allocated memory.
 * 
 * @return 0 upon successful execution, 1 on allocation failure.
 */
int main(void) {
  int matrix[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
  int **transpose = transposeMatrix(3, 3, matrix);
  if (transpose == NULL)
    return 1;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      printf("%d ", transpose[i][j]);
    }
    printf("\n");
  }
  for (int i = 0; i < 3; i++)
    free(transpose[i]);

  free(transpose);
  return 0;
}