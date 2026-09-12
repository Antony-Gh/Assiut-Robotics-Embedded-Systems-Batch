/*

A nested structure is a structure used as a field inside another structure.

struct Date {
    int day;
    int month;
    int year;
};

struct Student {
    char name[20];
    struct Date birth_date;
};

Access nested fields using the dot operator:

student.birth_date.year

*/