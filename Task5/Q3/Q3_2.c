
#include <stdio.h>
#include <stddef.h>

size_t integer_sqrt(size_t n)
{
    size_t root = 0;

    while (root + 1 <= n / (root + 1))
        root++;

    return root;
}

int jump_search(const int arr[], size_t n, int target)
{
    if (n == 0)
        return -1;

    size_t step = integer_sqrt(n);

    if (step == 0)
        step = 1;

    size_t start = 0;
    size_t end = step;

    while (start < n)
    {
        if (arr[end < n ? end : n - 1] >= target)
            break;

        start = end;

        if (start >= n)
            return -1;

        if (step > n - start)
            end = n;
        else
            end = start + step;
    }

    for (size_t i = start; i < end; i++)
    {
        if (arr[i] == target)
            return (int)i;

        if (arr[i] > target)
            break;
    }

    return -1;
}

int main(void)
{
    int arr[] = {1, 3, 5, 7, 9, 11, 13, 15};
    size_t n = sizeof(arr) / sizeof(arr[0]);

    int target = 11;
    int index = jump_search(arr, n, target);

    if (index != -1)
        printf("Found at index %d\n", index);
    else
        printf("Not found\n");

    return 0;
}