# C Tasks — Week 4 (C17 Solutions)

## Overview

Task 4 focuses on defining type aliases with `typedef`, organizing them in a header file, and using those aliases with structures.

The task contains two questions:

1. Create a header file containing typedefs for C’s signed and unsigned integer types.
2. Revisit Week 3, Question 2, using structure typedefs and the types defined in the header file.

> **Note:** Week 3, Question 2 was not included in the provided task description. The second section below explains how to apply the required typedefs, but its exact structure and output depend on the original question.

## Q4.1 — Create a header file for signed and unsigned types

### Objective

Create a reusable header file named `types.h` that provides convenient aliases for standard signed and unsigned integer types.

### Types to include

| Standard C type      | Example alias |
| -------------------- | ------------- |
| `signed char`        | `s_char`      |
| `short`              | `s_short`     |
| `int`                | `s_int`       |
| `long`               | `s_long`      |
| `long long`          | `s_long_long` |
| `unsigned char`      | `u_char`      |
| `unsigned short`     | `u_short`     |
| `unsigned int`       | `u_int`       |
| `unsigned long`      | `u_long`      |
| `unsigned long long` | `u_long_long` |

The header can also provide aliases for fixed-width integer types, such as 8-bit, 16-bit, 32-bit, and 64-bit signed and unsigned integers, when those types are available through the standard `<stdint.h>` header.

### Header-file requirements

* Use an include guard to prevent the header from being included more than once.
* Include the standard header needed for any fixed-width types used.
* Keep the typedef names consistent throughout the project.
* Include `types.h` in source files that need these aliases.

### Important C17 note

C does not define a separate built-in type for every possible integer width. The widths of types such as `short`, `int`, and `long` can vary by implementation. Fixed-width types such as `int32_t` are available only when the implementation provides a type with that exact width.

The typedef aliases do not create new types; they provide alternative names for existing types.

## Q4.2 — Solve Week 3, Question 2 using typedefs and structures

### Objective

Reimplement the Week 3, Question 2 task while meeting these requirements:

* Define the required structure using `typedef`.
* Use the aliases declared in `types.h` for applicable fields and variables.
* Keep the structure definition and data handling consistent with the original question.
* Preserve the input and output requirements of Week 3, Question 2.

### Recommended approach

1. Review the original Week 3, Question 2 requirements and identify the data that must be stored.
2. Define a structure containing the required fields.
3. Give the structure a typedef alias so it can be declared without repeatedly writing the `struct` keyword.
4. Use the aliases from `types.h` for suitable integer fields.
5. Implement the original task’s required operations, such as reading, displaying, searching, or processing the data.
6. Verify the result against the original sample output.

### What is still needed

The exact implementation cannot be finalized from the Week 4 description alone because it refers to Week 3, Question 2 without repeating it.

To complete this section accurately, provide the original Week 3, Question 2 statement and any required sample input/output. That will determine the structure’s fields and the program’s behavior.

## Key learning outcomes

After completing Task 4, you should understand:

* How `typedef` creates alternative names for existing C types.
* How to organize reusable type definitions in a header file.
* How to use typedef aliases in structures and variables.
* Why consistent type definitions improve readability and maintainability.
* How to separate shared declarations from program logic.
