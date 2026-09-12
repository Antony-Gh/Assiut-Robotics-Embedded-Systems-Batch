

#include <stdio.h>

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