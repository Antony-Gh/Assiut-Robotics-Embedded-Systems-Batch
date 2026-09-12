/*

Structure: Each member has its own storage. All members can hold values at the same time.

Union: Members share the same storage. It is generally used to hold one member’s value at a time.

struct Data {
    int number;
    float decimal;
};

union Value {
    int number;
    float decimal;
};

The size of a structure is large enough for all its members plus any padding.
The size of a union is large enough for its largest member, plus any required alignment.

*/