/**
 * Demonstrates the ownership rules for malloc, calloc, realloc, and free.


Q5.1 — Difference between malloc(), calloc(), realloc(), and free()

Function

Purpose

malloc(size)

Allocates a block of memory. Its contents are uninitialized.

calloc(n, size)

Allocates memory for n elements and initializes all bytes to zero.

realloc(ptr, size)

Resizes an existing allocation, possibly moving it to a new address.

free(ptr)

Releases previously allocated memory.

Important rules:

Always check whether an allocation returned NULL.

Do not use memory after freeing it.

Do not free the same allocation twice.

realloc preserves the existing data up to the smaller of the old and new sizes.

If realloc fails for a nonzero size, the original allocation remains valid.

calloc initializes bytes to zero; for ordinary integer arrays, that means all
elements are zero.
 */

#include <stdlib.h>

int main(void) {

  /* Allocate memory for 5 integers */
  int *a = malloc(5 * sizeof *a);

  /* Allocate memory for 5 integers and initialize all bytes to zero */
  int *b = calloc(5, sizeof *b);

  /* Resize the allocation to 10 integers without losing the original on failure. */
  int *temp = realloc(a, 10 * sizeof *a);

  /* If the reallocation is successful, update the pointer */
  if (temp != NULL)
    a = temp;

  /* temp aliases a after success, so free the allocation exactly once. */
  free(a);
  free(b);

  return 0;
}