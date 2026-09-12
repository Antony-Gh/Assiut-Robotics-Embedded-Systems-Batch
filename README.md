# C Programming Tasks

A structured collection of C programming exercises completed throughout a five-week C programming phase.

The repository progresses from fundamental C concepts and memory manipulation to data structures, bitwise programming, dynamic memory management, searching, sorting, and algorithmic problem-solving.

> **Language:** C
> **Standard:** C17
> **Author:** Anthony Gaius
> **Team:** Assiut Robotics Team

---

## About This Repository

This repository documents my progress through a structured C programming training phase.

Each week introduces new concepts and builds upon the previous weeks. The exercises are designed not only to practice C syntax, but also to develop:

* Problem-solving skills
* Memory awareness
* Modular programming
* Algorithmic thinking
* Data structure understanding
* Time and space complexity analysis
* Efficient implementation techniques

Each week contains its own `README.md` with explanations of the tasks, concepts, approaches, and important notes.

---

# Weekly Tasks

| Week       | Main Topics                                                              | Documentation                    |
| ---------- | ------------------------------------------------------------------------ | -------------------------------- |
| **Week 1** | Data Types, Operators, Integer Ranges, Bit Manipulation, Loops, `switch` | [Week 1 README](Week1/README.md) |
| **Week 2** | Pointers, Arrays, Strings, 2D Arrays, Bubble Sort, Matrix Operations     | [Week 2 README](Week2/README.md) |
| **Week 3** | Structures, `typedef`, `enum`, `union`, Preprocessor, Complex Numbers    | [Week 3 README](Week3/README.md) |
| **Week 4** | Bitwise Algorithms, Macros, Header Files, Dynamic Memory Allocation      | [Week 4 README](Week4/README.md) |
| **Week 5** | Searching, Sorting, GCD, Fibonacci, Divide-and-Conquer, Complexity       | [Week 5 README](Week5/README.md) |

---

# Week 1 — C Fundamentals

Week 1 introduces the fundamental building blocks of the C language.

### Topics Covered

* C data types:

  * Basic types
  * Derived types
  * User-defined types
* C operators:

  * Arithmetic
  * Relational
  * Logical
  * Assignment
  * Increment and decrement
  * Bitwise
  * Conditional
  * `sizeof`
  * Address and indirection
  * Member access
  * Type casting
  * Comma operator
* Integer ranges:

  * 8-bit unsigned integers
  * 16-bit signed integers
* Bit manipulation:

  * Clearing a bit
  * Setting a bit
  * Toggling a bit
* Processing an unknown number of inputs without arrays
* Finding:

  * Minimum
  * Maximum
  * Average
* `do-while` loops
* `switch` statements
* Day-of-week selection
* Arithmetic calculator
* Leap-year detection

### Main Learning Goal

Understand the basic building blocks of C and become comfortable with operators, control flow, functions, and low-level bit manipulation.

[**View Week 1 README →**](Week1/README.md)

---

# Week 2 — Pointers, Arrays, Strings, and Matrices

Week 2 builds on the fundamentals by introducing pointers and more advanced data processing.

### Topics Covered

#### Pointers

* Swapping values using pointers
* Reversing strings using pointers
* Passing values by reference
* Returning multiple results through pointer parameters

#### Arrays

* Minimum and maximum elements
* Average calculation
* Reversing an array in place
* Working with arrays without allocating another array

#### Sorting

* Bubble Sort
* Reusing a custom swap function

#### Two-Dimensional Arrays

* Matrix input and output
* Minimum and maximum matrix elements
* Matrix average
* Row sums
* Maximum row sum
* Passing 2D arrays to functions

#### Strings

* Understanding C strings
* Null terminator
* Manual string concatenation
* Manual string copying
* Palindrome detection
* Ignoring spaces, punctuation, and capitalization
* Replacing whitespace with underscores

#### Bonus

* Splitting strings using delimiters
* Matrix transpose
* Symmetric matrix detection
* Matrix multiplication
* Matrix multiplication dimension rules

### Main Learning Goal

Develop a strong understanding of pointers, arrays, strings, and two-dimensional data while practicing in-place algorithms and function-based programming.

[**View Week 2 README →**](Week2/README.md)

---

# Week 3 — Structures, Unions, Enums, and Preprocessor

Week 3 introduces more advanced C data modeling and memory concepts.

### Topics Covered

#### Structures

* Structures containing different data types
* Nested structures
* Structure assignment
* `typedef` with structures
* Structures versus unions
* Pointers to structures
* Structure padding
* Structure packing

#### Student Management

A student record containing:

* Student name
* Arabic mark
* English mark
* Mathematics mark

Students are sorted according to:

1. Total marks in descending order
2. Name alphabetically when totals are equal

#### Complex Numbers

A complex-number structure is used to perform:

* Printing
* Addition
* Subtraction
* Multiplication

The operations are then used to evaluate a compound complex-number expression.

#### Enumerations

* Traffic-light states
* Mapping enum values to actions
* Employee roles
* Using enums with structures

#### Preprocessor

Understanding:

* `#pragma`
* `#ifdef`
* `#ifndef`
* `#pragma once`
* Include guards
* Differences between portable include guards and compiler-specific directives

#### Macros

Using a constant representation of π for calculating:

* Great-circle circumference
* Surface area
* Sphere volume

#### Unions

* Storing different data types in shared memory
* Understanding shared storage
* Memory efficiency in embedded systems
* Tagged unions
* Union-based polymorphic behavior

### Main Learning Goal

Learn how C can represent complex data using structures, unions, and enumerations while understanding how memory layout and the preprocessor affect programs.

[**View Week 3 README →**](Week3/README.md)

---

# Week 4 — Bitwise Programming, Macros, and Dynamic Memory

Week 4 moves deeper into low-level C programming and memory management.

### Topics Covered

#### Bitwise Operations

* Checking whether a number is even or odd
* Swapping values without a temporary variable
* Checking whether a number is a power of two
* Counting set bits
* Finding a non-repeating element in an array
* Reversing the bits of an integer
* Multiplication using bitwise operations
* Finding a missing number using XOR

#### Macros

* `SQUARE`
* Generic `SWAP`
* Maximum element of an array
* Prime-checking macro

#### Header Files and `typedef`

* Creating custom header files
* Defining integer type aliases
* Rewriting the Week 3 student problem using the defined types

#### Dynamic Memory Allocation

Understanding the purpose and differences between:

* `malloc`
* `calloc`
* `realloc`
* `free`

Applications include:

* Dynamically allocated string copying
* Dynamically allocated 2D arrays
* Jagged matrices
* Dynamic matrix transpose
* Correct memory cleanup

#### Bonus

* Finding two non-repeating elements using bitwise operations
* Prime checking with a macro
* Dynamic student records with variable numbers of students

### Main Learning Goal

Become comfortable with low-level bit manipulation, reusable macros, header organization, and safe dynamic memory management.

[**View Week 4 README →**](Week4/README.md)

---

# Week 5 — Searching, Sorting, and Algorithms

Week 5 focuses heavily on algorithms, efficiency, and complexity analysis.

### Topics Covered

#### Mathematical and String Problems

* Greatest Common Divisor using the Euclidean algorithm
* Comparing numeric strings by their numerical value
* Comparing strings lexicographically
* Determining maximum values under different comparison rules

#### Sorting Algorithms

* Bubble Sort
* Insertion Sort
* Selection Sort
* Merge Sort
* Stable sorting
* Comparing sorting algorithms using time and space complexity

#### Searching Algorithms

* Linear Search
* Binary Search
* Jump Search
* Finding the number of elements smaller than a target
* Finding the number of elements greater than a target
* Counting occurrences using binary-search boundaries

#### Fibonacci

* Finding the nth Fibonacci number
* Iterative implementation
* Avoiding unnecessary recursive computation
* Reducing memory usage

#### Number Guessing

Binary-search based guessing:

* Range from 1 to 1000
* Maximum of 10 yes/no questions

Ternary-search based guessing:

* Range from 1 to 1000
* Three possible responses
* Maximum of 7 questions

#### 2D Point Sorting

Sorting points according to their distance from a specified reference point.

The task emphasizes improving upon quadratic-time approaches.

Squared distance can be used instead of the actual Euclidean distance because the square-root operation does not change the ordering of nonnegative distances.

#### Merge Sort Applications

* Sorting arrays
* Sorting 2D points
* Achieving O(n log n) sorting performance
* Using stable sorting where required

#### Bonus Problems

Selected algorithmic problems include:

* Sorting students by name and then score
* Finding a missing number using XOR
* Calculating the MEX of an array
* Finding two elements whose sum equals a target

### Main Learning Goal

Develop algorithmic thinking and understand how the choice of algorithm affects time and memory requirements.

[**View Week 5 README →**](Week5/README.md)

---

# Algorithm Complexity Overview

The later weeks introduce explicit analysis of algorithm efficiency.

| Algorithm / Technique                 | Typical Time Complexity |               Extra Space |
| ------------------------------------- | ----------------------: | ------------------------: |
| Linear Search                         |                    O(n) |                      O(1) |
| Bubble Sort                           |                   O(n²) |                      O(1) |
| Selection Sort                        |                   O(n²) |                      O(1) |
| Insertion Sort                        |        O(n²) worst case |                      O(1) |
| Binary Search                         |                O(log n) |                      O(1) |
| Jump Search                           |                   O(√n) |                      O(1) |
| Merge Sort                            |              O(n log n) |                      O(n) |
| Euclidean GCD                         |                O(log n) |                      O(1) |
| Iterative Fibonacci                   |                    O(n) |                      O(1) |
| XOR Missing Number                    |                    O(n) |                      O(1) |
| Two-Pointer Pair Search after sorting |              O(n log n) | Depends on sorting method |

The goal is not simply to make a solution work, but to understand **why one solution can be more efficient than another**.

---

# Skills Developed

Throughout the five weeks, the exercises progressively develop the following skills:

### C Fundamentals

* Data types
* Operators
* Expressions
* Control flow
* Functions
* Input and output

### Memory and Pointers

* Addresses
* Dereferencing
* Pointer parameters
* In-place operations
* Dynamic allocation
* Memory cleanup

### Data Structures

* Arrays
* Strings
* Matrices
* Structures
* Nested structures
* Unions
* Enumerations

### Low-Level Programming

* Bitwise operations
* Bit masks
* XOR techniques
* Bit shifting
* Memory layout
* Padding and packing

### Preprocessor and Code Organization

* Header files
* Include guards
* Conditional compilation
* Macros
* `typedef`

### Algorithms

* GCD
* Fibonacci
* Searching
* Sorting
* Matrix operations
* Two-pointer techniques
* Divide-and-conquer
* XOR-based algorithms

### Complexity Analysis

* Big-O notation
* Time complexity
* Space complexity
* Comparing algorithms
* Choosing appropriate algorithms for a problem

---

# Repository Structure

```text
C-Programming/
│
├── Week1/
│   └── README.md
│
├── Week2/
│   └── README.md
│
├── Week3/
│   └── README.md
│
├── Week4/
│   └── README.md
│
├── Week5/
│   └── README.md
│
└── README.md
```

Each week is kept independent so that the exercises can be studied, compiled, and reviewed separately.

---

# Progression

The five weeks follow a progressive learning path:

**Week 1**

Fundamentals
↓
**Week 2**

Pointers, Arrays & Strings
↓
**Week 3**

Data Structures & Memory Layout
↓
**Week 4**

Bitwise Programming & Dynamic Memory
↓
**Week 5**

Algorithms & Complexity

This progression moves from basic C syntax toward increasingly low-level and algorithmically efficient solutions.

---

# Learning Philosophy

The purpose of these tasks is not only to produce working programs.

Each exercise is an opportunity to understand:

* How data is represented in memory
* How functions interact with memory
* How pointers allow direct data manipulation
* How different data structures organize information
* How bitwise operations can solve problems efficiently
* How dynamic memory should be managed safely
* How algorithms can be compared mathematically
* How to choose an appropriate solution based on constraints

The final goal is to move from **writing code that works** to **writing code that is correct, efficient, maintainable, and understandable**.

---

# Author

**Anthony Gaius**

**Assiut Robotics Team**

---

## Course

**C Programming — 5 Week Training Phase**

**Language:** C
**Standard:** C17

---
