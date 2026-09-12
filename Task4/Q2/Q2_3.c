
#include <stdio.h>
#include <stddef.h>

int find_missing(const int arr[], size_t size)
{
    int missing = (int)size;

    for (size_t i = 0; i < size; i++)
    {
        missing ^= (int)i;
        missing ^= arr[i];
    }

    return missing;
}

int main(void)
{
    int arr[] = {3, 0, 1};
    size_t size = sizeof(arr) / sizeof(arr[0]);

    printf("Missing number: %d\n",
           find_missing(arr, size));

    return 0;
}