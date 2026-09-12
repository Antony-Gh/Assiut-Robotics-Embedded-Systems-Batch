/*

C copies the structure’s fields by value. Arrays and other fields are copied as part of the structure assignment.

struct Point {
    int x;
    int y;
};

struct Point a = {3, 4};
struct Point b;

b = a;  // b.x becomes 3 and b.y becomes 4

If a structure contains a pointer, the pointer value is copied—not the memory it points to. This is a shallow copy.

*/