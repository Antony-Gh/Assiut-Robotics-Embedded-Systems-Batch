/* Task 3, Q5.3: print a tagged union through the active member type. */
#include <stdio.h>

typedef enum { VALUE_INT, VALUE_FLOAT, VALUE_STRING } ValueType;

typedef union {
  int integer;
  float decimal;
  const char *string;
} Value;

/**
 * @brief Prints the value held within a tagged union based on its type.
 * 
 * A tagged union uses an enum (tag) to keep track of which member is currently
 * active, ensuring the union is accessed safely.
 * 
 * @param value The union containing the data.
 * @param type The tag specifying which type the union is currently holding.
 */
void print_value(Value value, ValueType type) {
  switch (type) {
  case VALUE_INT:
    printf("Integer: %d\n", value.integer);
    break;

  case VALUE_FLOAT:
    printf("Float: %.2f\n", value.decimal);
    break;

  case VALUE_STRING:
    if (value.string != NULL)
      printf("String: %s\n", value.string);
    else
      puts("String: (null)");
    break;

  default:
    puts("Invalid value type");
    break;
  }
}

/**
 * @brief The main entry point of the program.
 * 
 * Demonstrates using a tagged union by assigning different types of data to it
 * and passing the corresponding tag to the print_value function.
 * 
 * @return 0 upon successful execution.
 */
int main(void) {
  Value value;

  value.integer = 42;
  print_value(value, VALUE_INT);

  value.decimal = 3.14f;
  print_value(value, VALUE_FLOAT);

  value.string = "Hello C17";
  print_value(value, VALUE_STRING);

  return 0;
}