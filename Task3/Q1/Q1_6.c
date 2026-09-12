/*

Use -> to access a field through a structure pointer. It is equivalent to dereferencing the pointer and using ..

#include <stdio.h>

typedef struct {
    int x;
    int y;
} Point;

int main(void)
{
    Point p = {3, 4};
    Point *ptr = &p;

    printf("%d %d\n", ptr->x, ptr->y);
    return 0;
}

*/