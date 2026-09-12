
/*

# C Operators

A reference of the operator categories in C, with example usage.

## Overview table

| Category | Operators | Example |
|---|---|---|
| Arithmetic | `+` `-` `*` `/` `%` | `sum = a + b;` |
| Relational | `<` `<=` `>` `>=` `==` `!=` | `a >= b` |
| Logical | `&&` `\|\|` `!` | `a > 0 && b > 0` |
| Assignment | `=` `+=` `-=` `*=` `/=` `%=` | `a += 5;` |
| Increment / decrement | `++` `--` | `i++;` |
| Bitwise | `&` `\|` `^` `~` `<<` `>>` | `mask = value & 1;` |
| Conditional | `?:` | `max = (a > b) ? a : b;` |
| `sizeof` | `sizeof` | `sizeof(int)` |
| Address / indirection | `&` `*` | `ptr = &value;` , `*ptr` |
| Member access | `.` `->` | `student.id` , `ptr->id` |
| Type cast | `(type)` | `(double)sum` |
| Comma | `,` | `i = 1, j = 2;` |

---

## Categories in detail

### Arithmetic operators

Perform basic mathematical operations.

| Operator | Meaning | Example |
|---|---|---|
| `+` | Addition | `sum = a + b;` |
| `-` | Subtraction | `diff = a - b;` |
| `*` | Multiplication | `prod = a * b;` |
| `/` | Division | `quot = a / b;` |
| `%` | Modulus (remainder) | `rem = a % b;` |

Notes:

- `/` on two integers performs **integer division** (`7 / 2 == 3`).
- `%` is only valid for integer types.
- Division or modulus by zero is undefined behaviour.

### Relational operators

Compare two values; yield `1` (true) or `0` (false).

| Operator | Meaning | Example |
|---|---|---|
| `<` | Less than | `a < b` |
| `<=` | Less than or equal | `a <= b` |
| `>` | Greater than | `a > b` |
| `>=` | Greater than or equal | `a >= b` |
| `==` | Equal to | `a == b` |
| `!=` | Not equal to | `a != b` |

Notes:

- `==` is comparison, `=` is assignment — easy to confuse.
- Comparison of floating-point values with `==` is usually a bug; compare against an epsilon instead.

### Logical operators

Combine boolean expressions using short-circuit evaluation.

| Operator | Meaning | Example |
|---|---|---|
| `&&` | Logical AND | `a > 0 && b > 0` |
| `\|\|` | Logical OR | `a == 0 \|\| b == 0` |
| `!` | Logical NOT | `!found` |

Notes:

- `&&` stops evaluating as soon as an operand is false.
- `\|\|` stops evaluating as soon as an operand is true.
- Result is always `1` or `0`, not the operand value.

### Assignment operators

Store a value into a variable, optionally combined with an arithmetic operation.

| Operator | Meaning | Equivalent |
|---|---|---|
| `=` | Simple assignment | `a = b;` |
| `+=` | Add and assign | `a = a + b;` |
| `-=` | Subtract and assign | `a = a - b;` |
| `*=` | Multiply and assign | `a = a * b;` |
| `/=` | Divide and assign | `a = a / b;` |
| `%=` | Modulus and assign | `a = a % b;` |

Notes:

- Assignment is an expression: `while ((c = getchar()) != EOF)` is valid C.
- Compound assignment evaluates the left side only once, which matters for side-effecting expressions.

### Increment and decrement operators

Add or subtract `1` from a variable.

| Operator | Meaning | Example |
|---|---|---|
| `++` | Increment by 1 | `i++;` , `++i;` |
| `--` | Decrement by 1 | `i--;` , `--i;` |

Prefix vs. postfix:

- `++i` — increments first, then yields the new value.
- `i++` — yields the current value, then increments.

```c
int i = 5;
int a = ++i;    i is 6, a is 6 
int j = 5;
int b = j++;    j is 6, b is 5 

*/