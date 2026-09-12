
#include <stdio.h>

void swap(int *a, int *b) {
  int temp = *a;
  *a = *b;
  *b = temp;
}

void bubbleSort(int arr[], int size)
{
    for (int i = 0; i < size - 1; i++)
    {
        int swapped = 0;

        for (int j = 0; j < size - 1 - i; j++)
        {
            if (arr[j] > arr[j + 1])
            {
                swap(&arr[j], &arr[j + 1]);
                swapped = 1;
            }
        }

        if (!swapped)
            break;
    }
}

int main(void) {
  int arr[] = {5, 3, 8, 4, 2};
  int size = sizeof(arr) / sizeof(arr[0]);
  bubbleSort(arr, size);
  for (int i = 0; i < size; i++) {
    printf("%d ", arr[i]);
  }
  return 0;
}