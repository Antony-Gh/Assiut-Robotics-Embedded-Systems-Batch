
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>


typedef struct {
  size_t rows;
  size_t cols;
  int **data;
} Matrix;

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

int allocate_matrix(Matrix *m, size_t rows, size_t cols) {
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

void print_matrix(const Matrix *m) {
  for (size_t i = 0; i < m->rows; i++) {
    for (size_t j = 0; j < m->cols; j++)
      printf("%d ", m->data[i][j]);

    printf("\n");
  }
}

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