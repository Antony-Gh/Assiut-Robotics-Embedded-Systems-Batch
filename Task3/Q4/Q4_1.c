#include <stdio.h>

typedef enum { RED, YELLOW, GREEN } TrafficLight;

void print_action(TrafficLight light) {
  switch (light) {
  case RED:
    puts("Stop");
    break;
  case YELLOW:
    puts("Prepare to stop");
    break;
  case GREEN:
    puts("Go");
    break;
  default:
    puts("Invalid traffic-light value");
    break;
  }
}

int main(void) {
  print_action(RED);
  print_action(YELLOW);
  print_action(GREEN);

  return 0;
}