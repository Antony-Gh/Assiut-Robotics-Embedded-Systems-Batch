/* Task 3, Q3: perform integer complex arithmetic and exponentiation. */
#include <stdio.h>

typedef struct {
  int real;
  int imaginary;
} Complex;

/**
 * @brief Adds two complex numbers.
 * 
 * @param a The first complex number.
 * @param b The second complex number.
 * @return A new Complex number representing the sum.
 */
static Complex add(Complex a, Complex b) {
  Complex result = {a.real + b.real, a.imaginary + b.imaginary};
  return result;
}

/**
 * @brief Subtracts the second complex number from the first.
 * 
 * @param a The first complex number.
 * @param b The complex number to subtract.
 * @return A new Complex number representing the difference.
 */
static Complex subtract(Complex a, Complex b) {
  Complex result = {a.real - b.real, a.imaginary - b.imaginary};
  return result;
}

/**
 * @brief Multiplies two complex numbers.
 * 
 * Uses the formula: (a+bi)(c+di) = (ac - bd) + (ad + bc)i
 * 
 * @param a The first complex number.
 * @param b The second complex number.
 * @return A new Complex number representing the product.
 */
static Complex multiply(Complex a, Complex b) {
  Complex result = {a.real * b.real - a.imaginary * b.imaginary,
                    a.real * b.imaginary + a.imaginary * b.real};
  return result;
}

/**
 * @brief Prints a complex number in a readable format (e.g., "a + bi").
 * 
 * Automatically handles the sign of the imaginary part to avoid printing "a + -bi".
 * 
 * @param z The complex number to print.
 */
static void print_complex(Complex z) {
  if (z.imaginary < 0)
    printf("%d - %di\n", z.real, -z.imaginary);
  else
    printf("%d + %di\n", z.real, z.imaginary);
}

/**
 * @brief Computes the power of a complex number.
 * 
 * Uses exponentiation by squaring for efficiency.
 * 
 * @param base The complex number base.
 * @param exponent The non-negative integer exponent.
 * @return A new Complex number representing (base ^ exponent).
 */
static Complex power(Complex base, unsigned int exponent) {
  Complex result = {1, 0};

  while (exponent > 0) {
    if (exponent & 1u)
      result = multiply(result, base);

    exponent >>= 1;
    if (exponent > 0)
      base = multiply(base, base);
  }

  return result;
}

/**
 * @brief The main entry point of the program.
 * 
 * Evaluates the expression (a^7 + b) - c for complex numbers a, b, and c,
 * and prints the result.
 * 
 * @return 0 upon successful execution.
 */
int main(void) {
  Complex a = {2, 5};
  Complex b = {3, 7};
  Complex c = {4, 6};

  Complex result = subtract(add(power(a, 7), b), c);

  printf("Result: ");
  print_complex(result);

  return 0;
}