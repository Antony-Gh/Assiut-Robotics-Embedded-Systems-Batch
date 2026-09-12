/*

Padding is unused space the compiler may insert between structure members or at the end of a structure to satisfy alignment requirements.

Packing means arranging members with reduced or no padding.

struct Example {
    char c;
    int i;
};

This structure commonly has padding between c and i, but exact layout is implementation-dependent.

In portable C17, there is no standard way to disable structure padding.
Compiler-specific extensions or pragmas may request packed layouts, but they are not portable and can cause unaligned accesses.
For embedded communication or file formats, prefer explicitly serializing fields into a byte buffer rather than relying on a packed structure’s memory layout.

*/