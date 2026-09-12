#include <stdio.h>

typedef struct {
  int real;
  int imaginary;
} Complex;

static Complex add(Complex a, Complex b) {
  Complex result = {a.real + b.real, a.imaginary + b.imaginary};
  return result;
}

static Complex subtract(Complex a, Complex b) {
  Complex result = {a.real - b.real, a.imaginary - b.imaginary};
  return result;
}

static Complex multiply(Complex a, Complex b) {
  Complex result = {a.real * b.real - a.imaginary * b.imaginary,
                    a.real * b.imaginary + a.imaginary * b.real};
  return result;
}

static void print_complex(Complex z) {
  if (z.imaginary < 0)
    printf("%d - %di\n", z.real, -z.imaginary);
  else
    printf("%d + %di\n", z.real, z.imaginary);
}

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

int main(void) {
  Complex a = {2, 5};
  Complex b = {3, 7};
  Complex c = {4, 6};

  Complex result = subtract(add(power(a, 7), b), c);

  printf("Result: ");
  print_complex(result);

  return 0;
}