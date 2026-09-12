# C Programming — Week 2

## Overview

This week focuses on pointers, arrays, strings, and matrices in C. The tasks provide practice with passing data to functions, modifying values through pointers, processing arrays, and working with two-dimensional data.

---

## Q1: Pointers

### 1. Swap Two Numbers Using Pointers

A function can swap two integers by receiving their memory addresses.

* A pointer stores the address of a variable.
* Dereferencing a pointer accesses or modifies the value at that address.
* A temporary variable is used to preserve one value while the two values are exchanged.

### 2. Reverse a String Using Pointers

A string can be reversed in place by using two pointers:

* One pointer starts at the first character.
* The other moves to the last character before the null terminator.
* The characters at both positions are swapped.
* The pointers move toward the center until they meet or cross.

This method does not require another character array.

---

## Q2: Arrays

### 1. Find the Minimum, Maximum, and Average

A function can receive an integer array and its size, then calculate its minimum, maximum, and average.

* Initialize the minimum and maximum using the first array element.
* Visit each element and update the minimum or maximum when necessary.
* Add the elements to a running sum.
* Calculate the average by dividing the sum by the number of elements.

The function can return the results through pointer parameters. This allows one function to provide multiple results.

The array may contain positive numbers, negative numbers, or zero. The calculation must not assume that all values are positive.

### 2. Reverse an Array Without Another Array

To reverse an array in place:

1. Set one index to the beginning of the array.
2. Set another index to the end.
3. Swap the elements at those positions.
4. Move the first index forward and the second index backward.
5. Stop when the indices meet or cross.

The array is modified directly, so no second array is needed.

---

## Q3: Bubble Sort

Bubble Sort arranges array elements by repeatedly comparing neighboring elements.

* Compare each element with the one immediately after it.
* Swap them if they are in the wrong order.
* Repeat the process until the array is sorted.
* After each pass, the largest remaining unsorted value reaches its correct position.
* The algorithm can stop early if a complete pass makes no swaps.

The swap function from Q1 is reused to exchange neighboring elements.

For an ascending sort, smaller values appear before larger values.

---

## Q4: Two-Dimensional Arrays and Matrices

### 1. Read, Print, and Calculate Matrix Statistics

A matrix is a two-dimensional array containing rows and columns.

The program should:

1. Read the number of rows and columns.
2. Read each matrix element.
3. Print the matrix in row-and-column form.
4. Find the minimum and maximum elements.
5. Calculate the average of all matrix elements.

The average is the sum of every element divided by the total number of elements, which is the number of rows multiplied by the number of columns.

The calculation must support positive values, negative values, and zero.

### 2. Calculate Every Row Sum and the Maximum Row Sum

The sum of a row is the total of all elements in that row.

* Calculate each row sum separately.
* Store the row sums in an output array.
* Compare the row sums to find the maximum row sum.

The function can receive the matrix dimensions and the matrix itself, then return the row sums through an output array and the maximum through a pointer parameter.

Using a wider integer type for sums helps reduce the risk of overflow when adding many integers.

---

## Q5: Strings

### 1. What Is a String in C?

A string in C is a sequence of characters stored in a character array and terminated by a null character.

### 2. What Is the Null Terminator?

The null terminator is the character `'\0'`. It marks the end of a C string.

It is necessary because C string operations use the null terminator to determine where the string ends. Without it, an operation may continue reading beyond the intended character array, causing undefined behavior.

For example, the string `"cat"` requires space for three letters and the null terminator.

### 3. Concatenate Two Strings Without `strcat()`

Concatenation means appending the contents of one string to the end of another.

A loop can:

* Find the end of the destination string.
* Copy characters from the source string into the destination.
* Add a null terminator after the copied characters.

The destination array must have enough capacity for both strings and the final null terminator. A safe function should check the available capacity before copying.

### 4. Copy a String Without `strcpy()`

A string can be copied character by character using a loop.

The copying process must include the null terminator so that the destination is also a valid C string.

The destination array must be large enough to hold the complete source string.

### 5. Check Whether a String Is a Palindrome

A palindrome reads the same forward and backward.

For this task, the comparison ignores:

* Spaces and other whitespace
* Punctuation
* Uppercase and lowercase differences

The function uses two positions:

* One begins at the start of the string.
* The other begins at the end.
* Non-alphanumeric characters are skipped.
* The remaining characters are compared without considering case.
* If all matching pairs are equal, the string is a palindrome.

Examples:

* `madam`
* `racecar`
* `level`
* `A man, a plan, a canal, Panama`
* `Was it a car or a cat I saw?`
* `121`
* `12321`

### 6. Replace Whitespace With Underscores

The function visits each character in the string.

Whenever it finds a whitespace character, it replaces that character with an underscore.

For example:

* Input: `This is a test string!`
* Output: `This_is_a_test_string!`

This operation replaces whitespace; it does not remove it.

---

# Bonus Questions

## Bonus Q1: Split a String by a Delimiter

Splitting divides a string into smaller parts using a delimiter, such as a comma or a space.

The `strtok()` function can be used to find tokens separated by delimiters.

Important points:

* `strtok()` modifies the original string by replacing delimiters with null terminators.
* The first call receives the string.
* Later calls continue tokenizing the same string.
* Consecutive delimiters are treated as separators, and empty tokens are not returned.
* The original string should be copied first if it must remain unchanged.

For example, splitting `red,green,blue` using a comma produces three tokens: `red`, `green`, and `blue`.

## Bonus Q2: Transpose a Matrix

The transpose of a matrix is created by exchanging its rows and columns.

If the original matrix has `m` rows and `n` columns, its transpose has `n` rows and `m` columns.

Each original element at row `i`, column `j` moves to row `j`, column `i` in the transpose.

For a dynamically allocated transpose:

* Allocate space for the transposed rows.
* Allocate space for each row's elements.
* Copy each element to its transposed position.
* Check whether each allocation succeeds.
* Free all allocated memory when it is no longer needed.

## Bonus Q3: Check Whether a Matrix Is Symmetric

A matrix is symmetric if it is square and equal to its transpose.

This means each element must equal the element mirrored across the main diagonal.

A matrix cannot be symmetric unless it has the same number of rows and columns.

The function compares each element with its mirrored element. If any pair differs, the matrix is not symmetric.

## Bonus Q4: Multiply Two Matrices

Matrix multiplication combines rows of the first matrix with columns of the second matrix.

### Rules

* The number of columns in the first matrix must equal the number of rows in the second matrix.
* If the first matrix has dimensions `m × n` and the second has dimensions `n × p`, the result has dimensions `m × p`.
* Each result element is the sum of the products of corresponding elements from one row of the first matrix and one column of the second matrix.
* Matrix multiplication is not generally commutative: multiplying A by B does not necessarily produce the same result as multiplying B by A.

### Technique

For every result position:

1. Select a row from the first matrix.
2. Select a column from the second matrix.
3. Multiply corresponding elements.
4. Add the products together.
5. Store the sum in the result matrix.

The process is repeated for every row and column combination.

### Example

First matrix, dimensions `2 × 3`:

|  1 |  2 |  3 |
| -: | -: | -: |
|  4 |  5 |  6 |

Second matrix, dimensions `3 × 2`:

|  7 |  8 |
| -: | -: |
|  9 | 10 |
| 11 | 12 |

The result has dimensions `2 × 2`:

|  58 |  64 |
| --: | --: |
| 139 | 154 |

For a detailed visual explanation, see [Matrix Multiplication — Math Is Fun](https://www.mathsisfun.com/algebra/matrix-multiplying.html).

---

## Key Learning Outcomes

After completing Week 2, I should be able to:

* Use pointers to modify variables through their addresses.
* Swap values and reverse strings or arrays in place.
* Pass arrays and their sizes to functions.
* Calculate minimum, maximum, average, and row sums.
* Sort arrays using Bubble Sort and a reusable swap function.
* Read, print, and process two-dimensional arrays.
* Understand C strings and the role of the null terminator.
* Concatenate, copy, compare, and transform strings without relying on the corresponding library functions.
* Check palindromes while ignoring spaces, punctuation, and letter case.
* Split strings using delimiters.
* Allocate and free memory for dynamically sized matrix operations.
* Calculate matrix transposes, check symmetry, and multiply compatible matrices.
