# C Tasks — Week 3

## Overview

Week 3 covers structures, unions, enums, typedefs, preprocessor directives, and basic data organization in C17.

This README describes the purpose, concepts, and expected behavior of each question **without including source code**.

## Q1 — Structures

### Q1.1 — Can a structure contain different data types?

Yes. A structure groups related fields, and its members can have different data types, such as characters, integers, and floating-point numbers.

### Q1.2 — What is a nested structure?

A nested structure is a structure used as a member of another structure. It is useful for representing data that naturally contains smaller groups of related information, such as a student record containing a date of birth.

### Q1.3 — What happens when one structure is assigned to another of the same type?

The members are copied by value. If a structure contains a pointer, only the pointer value is copied—not the memory it points to. This is known as a shallow copy.

### Q1.4 — What is the purpose of `typedef` with structures?

`typedef` creates an alias for a type. With structures, it allows variables to be declared using a shorter type name without repeatedly writing the `struct` keyword.

### Q1.5 — What is the difference between a structure and a union?

* **Structure:** Each member has its own storage, so all members can hold values simultaneously.
* **Union:** All members share the same storage, so it is generally used when only one member is active at a time.

A structure’s size is sufficient for all its members plus any padding. A union’s size is sufficient for its largest member, subject to alignment.

### Q1.6 — How is a pointer to a structure used?

A pointer can refer to a structure object. Its members can be accessed through the pointer using the arrow operator. This is useful when passing structures to functions without copying the entire structure.

### Q1.7 — What are padding and packing?

**Padding** is unused space the compiler may insert between structure members or at the end of a structure to satisfy alignment requirements.

**Packing** means reducing or removing padding. ISO C17 does not provide a standard way to disable structure padding. Compiler-specific packing options are not portable and may cause unaligned memory access.

For communication protocols and file formats, explicitly serialize fields into a byte buffer instead of relying on a structure’s in-memory layout.

---

## Q2 — Student records and sorting

### Requirements

* Store each student’s name and marks in Arabic, English, and Math.
* Read a variable number of students.
* Limit each name to 19 characters.
* Calculate each student’s total marks.
* Sort students by total marks in descending order.
* If two students have the same total, sort their names in ascending alphabetical order.

### Approach

Use a structure to group the student’s name and three subject marks. Allocate enough storage for the number of students entered.

Calculate each student’s total by adding the three marks. Compare students first by total marks and then by name when totals are equal. Bubble sort can be used to arrange the records according to these rules.

### Complexity

* Sorting time: **O(n²)**
* Extra sorting space: **O(1)**
* Storage for the student records: **O(n)**

The name limit requires space for 19 characters plus the terminating null character. A single-word input method will not accept names containing spaces.

---

## Q3 — Complex numbers

### Requirements

Represent a complex number using two integer fields:

* Real part
* Imaginary part

Provide functions to print a complex number and perform:

* Addition
* Subtraction
* Multiplication

### Complex-number rules

For two complex numbers \(a+bi\) and \(c+di\):

* Addition: \((a+c)+(b+d)i\)
* Subtraction: \((a-c)+(b-d)i\)
* Multiplication: \((ac-bd)+(ad+bc)i\)

### Required expression

Calculate:

$$
(2+5i)^7 + (3+7i) - (4+6i)
$$

The expected result is:

$$
-60423 + 116616i
$$

Exponentiation by squaring can calculate the seventh power using fewer multiplications than repeated multiplication.

**Note:** Integer complex-number calculations can overflow if the values become too large. Use a wider integer type or floating-point fields when larger results are possible.

---

## Q4 — Enums, employees, and preprocessor directives

### Q4.1 — Traffic-light enum

Define an enum with the values RED, YELLOW, and GREEN. A function should use a switch statement to print the corresponding action:

| Color  | Action          |
| ------ | --------------- |
| RED    | Stop            |
| YELLOW | Prepare to stop |
| GREEN  | Go              |

The switch statement should include a default case to handle values outside the defined enum values.

### Q4.2 — Employee records

Define an enum for employee roles:

* Engineer
* Manager
* HR

Define an employee structure containing a name and role. The program should read a variable number of employees and then print each employee’s details.

Validate the role input before converting it to the enum type. Include handling for invalid input.

### Q4.3 — Preprocessor directives

* **`#pragma`** sends an implementation-specific instruction to the compiler.
* **`#ifdef`** includes code when a specified macro is defined.
* **`#ifndef`** includes code when a specified macro is not defined.
* **`#pragma once`** is a widely supported but non-standard instruction asking the compiler to include a header only once per translation unit.

Include guards use `#ifndef`, `#define`, and `#endif` to prevent repeated header inclusion. They are portable standard preprocessor logic, while `#pragma once` is not part of ISO C17.

### Q4.4 — Sphere measurements

Define a constant for π and use it to calculate the following values from a sphere’s radius \(r\):

* Great-circle circumference: \(C = 2\pi r\)
* Surface area: \(A = 4\pi r^2\)
* Volume: \(V = \frac{4}{3}\pi r^3\)

The program should read the radius, validate that it is non-negative, and print the three results.

---

## Q5 — Unions

### Q5.1 — Store an integer and a character in a union

Define a union containing an integer member and a character member. Assign and print each member separately.

Because union members share storage, assigning a new member overwrites the previously stored representation. Reading a different member does not generally recover the value previously assigned.

### Q5.2 — Unions in embedded systems

Unions can reduce memory usage when a variable needs to represent different data types at different times, but only one value is active at once.

Possible uses include:

* Sensor readings with alternative formats
* Communication messages with different payload types
* State-dependent data in memory-constrained systems

A separate tag, often an enum, should identify which union member is currently valid.

### Q5.3 — Union-based polymorphic function

Define an enum identifying three supported value types:

* Integer
* Float
* String

Define a union containing those members. A function receives the union and its type tag, then uses a switch statement to print the correct member.

The function should handle an invalid tag. If the string member is a pointer, it should also handle a null pointer. The caller is responsible for ensuring that the tag matches the member most recently assigned.

---

## Key learning outcomes

After completing Week 3, you should understand:

* How structures group related data of different types.
* How nested structures, structure assignment, and structure pointers work.
* How `typedef` simplifies type declarations.
* How structures differ from unions.
* How padding and alignment affect structure layout.
* How to organize and sort arrays of structure records.
* How to represent and operate on complex numbers.
* How enums, switch statements, and preprocessor directives work.
* How unions can represent alternative data while sharing storage.
* Why a union should be paired with a tag that identifies its active member.
