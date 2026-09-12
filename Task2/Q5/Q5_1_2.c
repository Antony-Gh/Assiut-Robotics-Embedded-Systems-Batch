/*

1. What is a string in C?

A string in C is a sequence of characters stored in a character array and terminated by the null character '\0'.

2. What is the null terminator, and why is it necessary?

'\0' marks the end of a C string. String functions use it to know where the string ends.
Without it, they may continue reading beyond the array, causing undefined behavior.

For example, the string "cat" requires four character positions:

'c'  'a'  't'  '\0'

*/