# C Programming — Week 1

## Overview

This week introduces fundamental C data types, operators, bit manipulation, loops, pointers, and `switch` statements. The tasks focus on understanding how data is represented and processed in C, and how to solve problems using functions and control flow.

---

## Q1: C Data Types

C data types are commonly grouped into three categories: **basic types, derived types, and user-defined types**.

### 1. Basic / Fundamental Types

Basic types represent simple values.

| Type     | Purpose                                                   | Example use            |
| -------- | --------------------------------------------------------- | ---------------------- |
| `char`   | Stores a character or a small integer value               | A letter or character  |
| `int`    | Stores whole numbers                                      | A count or age         |
| `float`  | Stores decimal values with single precision               | A measurement          |
| `double` | Stores decimal values with greater precision than `float` | A precise calculation  |
| `_Bool`  | Stores a Boolean value, either zero or one                | A true/false condition |

The exact size and range of most C types depend on the compiler and platform.

### 2. Derived Types

Derived types are formed from other types.

| Type          | Purpose                                           | Example use                      |
| ------------- | ------------------------------------------------- | -------------------------------- |
| Array         | Stores multiple elements of the same type         | A list of marks                  |
| Pointer       | Stores the address of an object or function       | Passing data by reference        |
| Function type | Describes a function’s return type and parameters | A function that calculates a sum |

### 3. User-Defined Types

User-defined types help organize data and make declarations clearer.

| Type      | Purpose                                               | Example use                                 |
| --------- | ----------------------------------------------------- | ------------------------------------------- |
| `struct`  | Groups related fields, which may have different types | A student record                            |
| `union`   | Allows different members to share the same memory     | A value that may be an integer or a decimal |
| `enum`    | Defines named integer constants                       | Days of the week                            |
| `typedef` | Creates an alias for an existing type                 | Simplifying a complex type name             |

`typedef` creates an alias; it does not create a completely new, distinct type.

---

## Q2: C Operators and Integer Ranges

### 1. Types of C Operators

| Operator category       | Purpose                                                                            |
| ----------------------- | ---------------------------------------------------------------------------------- |
| Arithmetic              | Performs addition, subtraction, multiplication, division, and remainder operations |
| Relational              | Compares two values                                                                |
| Logical                 | Combines or reverses conditions                                                    |
| Assignment              | Assigns or updates a variable’s value                                              |
| Increment and decrement | Increases or decreases a value by one                                              |
| Bitwise                 | Performs operations on individual bits                                             |
| Conditional             | Selects one of two expressions based on a condition                                |
| `sizeof`                | Determines the size of a type or object in bytes                                   |
| Address and indirection | Obtains an address or accesses the value at an address                             |
| Member access           | Accesses members of structures and unions                                          |
| Type cast               | Converts a value to another type                                                   |
| Comma                   | Evaluates expressions in sequence                                                  |

### 2. Range of an 8-Bit Unsigned Integer

An 8-bit unsigned integer can represent values from **0 to 255**, inclusive.

It has 256 possible values because all eight bits are used to represent a nonnegative value.

### 3. Range of a 16-Bit Signed Integer

Assuming the usual two’s-complement representation, a 16-bit signed integer ranges from **−32,768 to 32,767**, inclusive.

---

## Q3: Bit Manipulation

This task performs three operations on a selected bit of an integer.

Bit positions are **zero-based**, meaning bit position zero is the least significant bit.

### Clear a Bit

Clearing a bit sets the selected bit to zero while leaving the other bits unchanged.

### Set a Bit

Setting a bit changes the selected bit to one while leaving the other bits unchanged.

### Toggle a Bit

Toggling a bit changes zero to one or one to zero.

The function receives the integer and bit position, then returns the three resulting values through output parameters. The bit position must be within the width of the unsigned integer type.

---

## Q4: Read Numbers Until the User Stops

The program repeatedly asks the user to enter an integer and whether to continue.

The process ends when the user enters `n` or `N`.

The program must calculate:

* The minimum entered number
* The maximum entered number
* The average of all entered numbers

The solution does not use an array. Instead, it updates the minimum, maximum, sum, and count after each input.

The program must support positive numbers, negative numbers, and zero. The average should be displayed with two digits after the decimal point.

The loop always reads at least one number before calculating the results, so the count is not zero.

---

## Q5: Switch Statements

### 1. Display the Day of the Week

The program accepts a number from 1 to 7 and uses a `switch` statement to display the corresponding day.

* 1 represents Monday.
* 2 represents Tuesday.
* 3 represents Wednesday.
* 4 represents Thursday.
* 5 represents Friday.
* 6 represents Saturday.
* 7 represents Sunday.

Any other number should produce an invalid-input message.

### 2. Arithmetic Calculator

The program accepts an arithmetic operator and two numbers, then performs the selected operation using a `switch` statement.

Supported operations:

* Addition
* Subtraction
* Multiplication
* Division

The program should check for division by zero and handle unsupported operators.

### 3. Determine Whether a Year Is a Leap Year

A leap year has 366 days, including February 29.

A year is a leap year if:

* It is divisible by 400, **or**
* It is divisible by 4 but not divisible by 100.

Examples of leap years include 2000, 2004, 2008, 2012, 2016, 2020, and 2024.

Examples of years that are not leap years include 1700, 1800, 1900, and 2100.

The task uses a `switch` statement to select the result based on the leap-year condition.

---

## Q4: Sample Output

The exact sample-output page was not included with the task. The following illustrates the expected format:

```text
Enter an integer: -5
Continue? (y/n): y
Enter an integer: 0
Continue? (y/n): y
Enter an integer: 10
Continue? (y/n): N

Minimum: -5
Maximum: 10
Average: 1.67
```

---

## Key Learning Outcomes

After completing Week 1, I should be able to:

* Distinguish between basic, derived, and user-defined C data types.
* Identify common C operators and understand their purposes.
* Determine the ranges of common fixed-width integer types.
* Use pointers to modify values and return multiple results from a function.
* Perform bitwise operations to clear, set, and toggle individual bits.
* Process a sequence of user inputs without storing them in an array.
* Track minimum, maximum, sum, count, and average values.
* Use `switch` statements to select between multiple cases.
* Validate user input and handle invalid operations.
* Apply the leap-year rules correctly.
