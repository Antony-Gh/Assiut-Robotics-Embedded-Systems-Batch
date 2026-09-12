/* Task 3, Q4.4: calculate sphere circumference, area, and volume. */
#include <stdio.h>

#define PI 3.14159265358979323846

/**
 * @brief The main entry point of the program.
 * 
 * Reads the radius of a sphere from the user and calculates its 
 * great-circle circumference, surface area, and volume, then prints them.
 * 
 * @return 0 upon successful execution, 1 on invalid input.
 */
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