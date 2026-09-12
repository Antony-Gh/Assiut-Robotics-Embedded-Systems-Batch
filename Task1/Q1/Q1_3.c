/*

User-defined types

Types created or named by the programmer.

Type | Example | When to use
struct | struct Student { int id; float grade; }; | Group related fields, possibly of different types
union | union Value { int i; float f; }; | Store one of several member types in shared memory
enum | enum Day { MON, TUE, WED }; | Give names to a set of integer constants
typedef | typedef unsigned int uint; | Create an alias that makes declarations clearer

typedef creates a type alias; it does not create a completely new, distinct type.

*/