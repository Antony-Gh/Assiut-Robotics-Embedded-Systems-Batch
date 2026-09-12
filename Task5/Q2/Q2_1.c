
#include <stdio.h>
#include <stddef.h>

/* Linear search algorithm */
/*
Time complexity: O(n) worst case, O(1) best case.
Space complexity: O(1).
*/
int linear_search(const int arr[], size_t n, int target)
{
    for (size_t i = 0; i < n; i++)
    {
        if (arr[i] == target)
            return (int)i;
    }

    return -1;
}

int main(void)
{
    int arr[] = {10, 20, 30, 40, 50};
    size_t n = sizeof(arr) / sizeof(arr[0]);

    int index = linear_search(arr, n, 30);

    if (index != -1)
        printf("Found at index %d\n", index);
    else
        printf("Not found\n");

    return 0;
}