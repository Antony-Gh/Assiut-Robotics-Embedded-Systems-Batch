/* Task 3, Q4.1: map traffic-light enum values to their actions. */
#include <stdio.h>

typedef enum { RED, YELLOW, GREEN } TrafficLight;

/**
 * @brief Prints the action to take for a given traffic light color.
 * 
 * @param light The current state of the traffic light (RED, YELLOW, or GREEN).
 */
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

/**
 * @brief The main entry point of the program.
 * 
 * Tests the print_action function by passing all possible traffic light states.
 * 
 * @return 0 upon successful execution.
 */
int main(void) {
  print_action(RED);
  print_action(YELLOW);
  print_action(GREEN);

  return 0;
}