
#include <stdio.h>
#include <stddef.h>

int find_single(const int arr[], size_t size)
{
    int result = 0;

    for (size_t i = 0; i < size; i++)
        result ^= arr[i];

    return result;
}

int main(void)
{
    int arr[] = {4, 1, 2, 1, 2};
    size_t size = sizeof(arr) / sizeof(arr[0]);

    printf("Unique element: %d\n",
           find_single(arr, size));

    return 0;
}