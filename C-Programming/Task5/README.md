# C Tasks — Week 5 (C17 Solutions)

This repository contains solutions and explanations for all Week 5 C programming tasks, including all four bonus questions.

The solutions use standard C17 and cover fundamental algorithms, searching, sorting, recursion, strings, and problem-solving techniques.

## Table of Contents

* [Q1 — GCD, Strings, and Bubble Sort](#q1--gcd-strings-and-bubble-sort)
* [Q2 — Searching, Sorting, and Fibonacci](#q2--searching-sorting-and-fibonacci)
* [Q3 — Binary Search, Jump Search, and Number Prediction](#q3--binary-search-jump-search-and-number-prediction)
* [Q4 — Ternary Prediction and Point Sorting](#q4--ternary-prediction-and-point-sorting)
* [Q5 — Merge Sort and Counting Occurrences](#q5--merge-sort-and-counting-occurrences)
* [Bonus Questions](#bonus-questions)
* [Complexity Summary](#complexity-summary)

## Q1 — GCD, Strings, and Bubble Sort

### Q1.1 — Greatest Common Divisor (GCD)

Find the greatest common divisor of two integers using the Euclidean Algorithm.

The algorithm repeatedly replaces the two numbers with the second number and the remainder of their division until the remainder becomes zero.

**Complexity**

* Time: O(log(min(a, b)))
* Space: O(1)

### Q1.2 — Numeric and Lexicographic Maximum

Given two strings representing nonnegative integers without leading zeros, find:

* The numerically greater value.
* The lexicographically greater string.

Numeric comparison uses string lengths first, followed by lexicographic comparison when the lengths are equal.

**Complexity**

* Time: O(k), where k is the length of the longer string.
* Space: O(1)

### Q1.3 — Bubble Sort

Sort an integer array in ascending order by repeatedly comparing adjacent elements and swapping them when they are out of order.

The implementation uses early stopping when a complete pass performs no swaps.

| Case    | Time Complexity |
| ------- | --------------- |
| Best    | O(n)            |
| Average | O(n²)           |
| Worst   | O(n²)           |

Space complexity: O(1).

---

## Q2 — Searching, Sorting, and Fibonacci

### Q2.1 — Linear Search

Search for a target value by checking array elements sequentially.

The function returns the index of the first occurrence or indicates that the target was not found.

**Complexity**

* Best-case time: O(1)
* Worst-case time: O(n)
* Space: O(1)

### Q2.2 — Insertion Sort and Selection Sort

**Insertion Sort**

Build a sorted portion of the array by inserting each new element into its correct position.

**Selection Sort**

Find the smallest element in the unsorted portion and place it at the beginning of that portion.

| Algorithm      | Best Time | Average / Worst Time | Extra Space |
| -------------- | --------- | -------------------- | ----------- |
| Insertion Sort | O(n)      | O(n²)                | O(1)        |
| Selection Sort | O(n²)     | O(n²)                | O(1)        |

Insertion Sort is particularly useful for small or nearly sorted arrays. Selection Sort performs fewer swaps but still requires quadratic comparisons.

### Q2.3 — Nth Fibonacci Number

Calculate the nth Fibonacci number using an iterative approach.

The sequence begins with 0 and 1, and each subsequent number is the sum of the previous two.

The iterative implementation avoids the repeated calculations of naive recursion.

**Complexity**

* Time: O(n)
* Space: O(1)

The implementation uses a 64-bit unsigned integer, so the result must fit within the supported range.

---

## Q3 — Binary Search, Jump Search, and Number Prediction

### Q3.1 — Binary Search and Element Counts

Search a sorted array for a target value using Binary Search.

The solution also determines:

* Whether the target exists in the array.
* The number of elements strictly smaller than the target.
* The number of elements strictly greater than the target.

Lower Bound and Upper Bound searches provide these counts efficiently, including when the target appears multiple times.

**Complexity**

* Time: O(log n)
* Space: O(1)

### Q3.2 — Jump Search

Search a sorted array by jumping through blocks of approximately √n elements.

Once a block containing the target is identified, perform a linear search within that block.

**Complexity**

* Time: O(√n)
* Space: O(1)

### Q3.3 — Predict a Number from 1 to 1000

Predict a number selected by the user through Yes/No questions.

The algorithm uses Binary Search, asking whether the chosen number is greater than a calculated midpoint.

The search interval is halved after each valid answer.

**Maximum number of questions: 10.**

---

## Q4 — Ternary Prediction and Point Sorting

### Q4.1 — Predict a Number Using Three Answers

Predict a number from 1 to 1000 using three possible responses:

| Answer | Meaning                                             |
| ------ | --------------------------------------------------- |
| 0      | The number is below the proposed range.             |
| 1      | The number is inside the proposed range, inclusive. |
| 2      | The number is above the proposed range.             |

The algorithm divides the remaining possibilities into approximately three groups and continues with the selected group.

When only two possible numbers remain, a special final question distinguishes between them.

**Maximum number of questions: 7.**

### Q4.2 — Sort 2D Points by Distance

Sort a collection of two-dimensional points according to their distance from a specified reference point.

Each point is represented by its x and y coordinates.

The implementation compares squared Euclidean distances instead of calculating square roots, preserving the same ordering while avoiding unnecessary calculations.

Merge Sort is used to achieve better performance than quadratic sorting algorithms.

**Complexity**

* Time: O(n log n)
* Extra space: O(n)
* Recursion stack: O(log n)

---

## Q5 — Merge Sort and Counting Occurrences

### Q5.1 — Merge Sort

Implement Merge Sort using the divide-and-conquer approach.

The algorithm divides the array into smaller subarrays, recursively sorts them, and merges the sorted results.

**Time complexity: O(n log n).**

### Q5.2 — Sort Points Using Merge Sort

Reuse the point-sorting algorithm from Q4.2 to sort points by their distance from a specified reference point.

The implementation uses Merge Sort and satisfies the required time complexity of O(n log n).

### Q5.3 — Count Occurrences Using Binary Search

Count the occurrences of a target value in a sorted array.

The algorithm finds the first position greater than or equal to the target and the first position strictly greater than the target.

Subtracting these positions gives the total number of occurrences.

**Complexity**

* Time: O(log n)
* Space: O(1)

---

## Bonus Questions

All four bonus questions are included.

### Bonus Q1 — Sort Students by Name and Score

Sort an array of student structures alphabetically by name.

When two students have the same name, sort them by score in ascending order.

Merge Sort is used to achieve O(n log n) time complexity.

The sorting algorithm is stable, preserving the relative order of students with identical names and scores.

### Bonus Q2 — Find the Missing Number

Find the missing number in an array containing n unique integers from 1 to n + 1.

The solution uses XOR to cancel out all present numbers and leave the missing value.

**Complexity**

* Time: O(n)
* Space: O(1)

This approach avoids sorting and provides a linear-time solution.

### Bonus Q3 — Minimum Excluded Value (MEX)

Find the smallest nonnegative integer that does not appear in the given array.

The solution sorts a copy of the array and scans it from zero upward to find the first missing value.

The original array remains unchanged.

**Complexity**

* Time: O(n log n)
* Extra space: O(n)

### Bonus Q4 — Find Two Numbers That Sum to a Target

Find two distinct array elements whose sum equals a specified target.

The solution first sorts the array using Merge Sort, then applies the two-pointer technique.

The pointers move toward each other according to whether their sum is smaller or greater than the target.

**Complexity**

* Sorting: O(n log n)
* Two-pointer search: O(n)
* Total time: O(n log n)
* Extra space: O(n)

---

## Complexity Summary

| Question | Algorithm                   | Time Complexity      | Extra Space |
| -------- | --------------------------- | -------------------- | ----------- |
| Q1.1     | Euclidean GCD               | O(log n)             | O(1)        |
| Q1.2     | String comparison           | O(k)                 | O(1)        |
| Q1.3     | Bubble Sort                 | O(n²) worst          | O(1)        |
| Q2.1     | Linear Search               | O(n) worst           | O(1)        |
| Q2.2     | Insertion Sort              | O(n²) worst          | O(1)        |
| Q2.2     | Selection Sort              | O(n²)                | O(1)        |
| Q2.3     | Iterative Fibonacci         | O(n)                 | O(1)        |
| Q3.1     | Binary Search               | O(log n)             | O(1)        |
| Q3.2     | Jump Search                 | O(√n)                | O(1)        |
| Q3.3     | Binary Prediction           | At most 10 questions | O(1)        |
| Q4.1     | Ternary Prediction          | At most 7 questions  | O(1)        |
| Q4.2     | Point Merge Sort            | O(n log n)           | O(n)        |
| Q5.1     | Merge Sort                  | O(n log n)           | O(n)        |
| Q5.3     | Binary Occurrence Count     | O(log n)             | O(1)        |
| Bonus Q1 | Student Merge Sort          | O(n log n)           | O(n)        |
| Bonus Q2 | XOR Missing Number          | O(n)                 | O(1)        |
| Bonus Q3 | Sort and Scan MEX           | O(n log n)           | O(n)        |
| Bonus Q4 | Merge Sort and Two Pointers | O(n log n)           | O(n)        |

## Learning Objectives

This assignment provides practice with:

* Standard C17 programming and function design.
* Mathematical algorithms and number representation.
* String manipulation and comparison.
* Bitwise operations and XOR-based problem solving.
* Searching and sorting algorithms.
* Recursion and divide-and-conquer techniques.
* Time and space complexity analysis.
* Structures, arrays, and dynamic memory management.
* Efficient problem-solving techniques for algorithmic challenges.

## Notes

* All solutions are intended to use standard C17.
* Arrays are sorted in ascending order unless stated otherwise.
* Searching algorithms that require sorted input assume the array is already sorted.
* Complexity analysis describes algorithmic growth as the input size increases.
* Integer overflow and memory allocation failures should be handled where applicable.
* The point-sorting and student-sorting solutions use Merge Sort for O(n log n) performance.

**Recommended study order:** Start with searching and simple sorting algorithms, then study Binary Search, recursion, Merge Sort, and finally the bonus problems. Understanding Merge Sort and the two-pointer technique will help with several questions in this assignment.
