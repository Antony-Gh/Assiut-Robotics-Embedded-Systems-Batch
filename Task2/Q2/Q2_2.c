#include <stdio.h>

void swap(int *a, int *b) {
  int temp = *a;
  *a = *b;
  *b = temp;
}

void reverseArray(int arr[], int size) {
  int left = 0;
  int right = size - 1;

  while (left < right) {
    swap(&arr[left], &arr[right]);
    left++;
    right--;
  }
}

int main(void) {
  int size;

  printf("Enter array size: ");
  if (scanf("%d", &size) != 1 || size <= 0)
    return 1;

  int arr[size];

  for (int i = 0; i < size; i++) {
    printf("Enter element %d: ", i + 1);
    scanf("%d", &arr[i]);
  }

  reverseArray(arr, size);

  printf("Reversed array: ");
  for (int i = 0; i < size; i++)
    printf("%d ", arr[i]);

  printf("\n");
  return 0;
}