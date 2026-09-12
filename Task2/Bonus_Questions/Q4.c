/* Task 2 bonus: multiply compatible matrices using row-column products. */
#include <stdio.h>

/**
 * @brief Multiplies two matrices (A and B) and stores the result in matrix C.
 * 
 * Calculates the dot product of rows from matrix A and columns from matrix B.
 * 
 * @param m The number of rows in matrix A and C.
 * @param n The number of columns in matrix A (and rows in matrix B).
 * @param p The number of columns in matrix B and C.
 * @param A The first input matrix.
 * @param B The second input matrix.
 * @param C The output matrix to store the result.
 * @return 1 on success, 0 on invalid dimensions.
 */
int multiplyMatrices(int m, int n, int p,
                     const int A[m][n],
                     const int B[n][p],
                     long long C[m][p])
{
    if (m <= 0 || n <= 0 || p <= 0)
        return 0;

    for (int i = 0; i < m; i++)
    {
        for (int j = 0; j < p; j++)
        {
            C[i][j] = 0;

            for (int k = 0; k < n; k++)
                C[i][j] += (long long)A[i][k] * B[k][j];
        }
    }

    return 1;
}

/**
 * @brief The main entry point of the program.
 * 
 * Defines two matrices, multiplies them using multiplyMatrices, and prints
 * the resulting matrix.
 * 
 * @return 0 upon successful execution.
 */
int main(void) {
  int A[2][3] = {{1, 2, 3}, {4, 5, 6}};
  int B[3][2] = {{7, 8}, {9, 10}, {11, 12}};
  long long C[2][2];
  multiplyMatrices(2, 3, 2, A, B, C);
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      printf("%lld ", C[i][j]);
    }
    printf("\n");
  }
  return 0;
}