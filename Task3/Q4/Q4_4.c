#include <stdio.h>

#define PI 3.14159265358979323846

int main(void) {
  double radius;

  printf("Enter radius: ");
  if (scanf("%lf", &radius) != 1 || radius < 0.0)
    return 1;

  double circumference = 2.0 * PI * radius;
  double area = 4.0 * PI * radius * radius;
  double volume = (4.0 / 3.0) * PI * radius * radius * radius;

  printf("Great-circle circumference: %.2f\n", circumference);
  printf("Surface area: %.2f\n", area);
  printf("Volume: %.2f\n", volume);

  return 0;
}