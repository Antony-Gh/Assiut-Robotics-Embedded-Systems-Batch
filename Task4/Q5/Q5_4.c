
/* Task 4, Q5.4: manage a matrix abstraction and create its transpose. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


typedef struct {
  size_t rows;
  size_t cols;
  int **data;
} Matrix;

/**
 * @brief Frees all dynamically allocated memory within a Matrix.
 * 
 * Iterates through each row pointer and frees it, then frees the array of 
 * row pointers. Sets the matrix dimensions to 0 to indicate it's empty.
 * 
 * @param m Pointer to the Matrix structure to free.
 */
void free_matrix(Matrix *m) {
  if (m == NULL)
    return;

  for (size_t i = 0; i < m->rows; i++)
    free(m->data[i]);

  free(m->data);

  m->data = NULL;
  m->rows = 0;
  m->cols = 0;
}

/**
 * @brief Allocates memory for a 2D matrix structure.
 * 
 * First allocates an array of pointers (for the rows), then for each row, 
 * it allocates an array of integers (for the columns).
 * 
 * @param m Pointer to an existing Matrix structure to populate.
 * @param rows The desired number of rows.
 * @param cols The desired number of columns.
 * @return 1 on successful allocation, 0 on failure.
 */
int allocate_matrix(Matrix *m, size_t rows, size_t cols) {
  if (m == NULL || rows == 0 || cols == 0 ||
      rows > SIZE_MAX / sizeof *m->data ||
      cols > SIZE_MAX / sizeof *m->data[0])
    return 0;

  m->rows = rows;
  m->cols = cols;
  m->data = calloc(rows, sizeof *m->data);

  if (m->data == NULL)
    return 0;

  for (size_t i = 0; i < rows; i++) {
    m->data[i] = malloc(cols * sizeof *m->data[i]);

    if (m->data[i] == NULL) {
      free_matrix(m);
      return 0;
    }
  }

  return 1;
}

/**
 * @brief Computes and returns the transpose of a given matrix.
 * 
 * The transpose is created by swapping rows and columns. It allocates a new 
 * Matrix internally, which the caller will be responsible for freeing.
 * 
 * @param m Pointer to the original Matrix.
 * @return A new Matrix structure containing the transposed data.
 */
Matrix transpose(const Matrix *m) {
  Matrix t = {0, 0, NULL};

  if (m == NULL || m->data == NULL)
    return t;

  if (!allocate_matrix(&t, m->cols, m->rows))
    return t;

  for (size_t i = 0; i < m->rows; i++) {
    for (size_t j = 0; j < m->cols; j++) {
      t.data[j][i] = m->data[i][j];
    }
  }

  return t;
}

/**
 * @brief Prints the elements of a matrix in a grid format.
 * 
 * @param m Pointer to the Matrix structure to print.
 */
void print_matrix(const Matrix *m) {
  for (size_t i = 0; i < m->rows; i++) {
    for (size_t j = 0; j < m->cols; j++)
      printf("%d ", m->data[i][j]);

    printf("\n");
  }
}

/**
 * @brief The main entry point of the program.
 * 
 * Prompts the user to enter matrix dimensions and elements, allocates memory,
 * computes its transpose, prints both matrices, and frees the allocated memory.
 * 
 * @return 0 upon successful execution, 1 on error.
 */
int main(void) {
  Matrix m = {0, 0, NULL};

  printf("Enter rows and columns: ");

  if (scanf("%zu %zu", &m.rows, &m.cols) != 2 || m.rows == 0 || m.cols == 0)
    return 1;

  size_t rows = m.rows;
  size_t cols = m.cols;

  m.rows = 0;
  m.cols = 0;

  if (!allocate_matrix(&m, rows, cols)) {
    printf("Memory allocation failed\n");
    return 1;
  }

  for (size_t i = 0; i < m.rows; i++) {
    for (size_t j = 0; j < m.cols; j++) {
      printf("matrix[%zu][%zu] = ", i, j);

      if (scanf("%d", &m.data[i][j]) != 1) {
        free_matrix(&m);
        return 1;
      }
    }
  }

  Matrix t = transpose(&m);

  if (t.data == NULL) {
    printf("Transpose allocation failed\n");
    free_matrix(&m);
    return 1;
  }

  printf("\nOriginal matrix:\n");
  print_matrix(&m);

  printf("\nTransposed matrix:\n");
  print_matrix(&t);

  free_matrix(&m);
  free_matrix(&t);

  return 0;
}