#include <stdio.h>

typedef enum { VALUE_INT, VALUE_FLOAT, VALUE_STRING } ValueType;

typedef union {
  int integer;
  float decimal;
  const char *string;
} Value;

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