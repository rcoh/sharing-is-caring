#include <unistd.h>
#include <stdio.h>
#include "../sic.h"

int main() {
  sic_init();
  char *foo = sic_malloc(50000);
  int i;
  if (sic_id() == 0) {
    for (i = 0; i < 128; i++) {
      if (i % 8 == 0) {
        foo[i] = 'a';
      }
    }
  }

  sic_barrier(0);
  for (i = 0; i < 128; i++) {
    if (i % 8 == 0) {
      if (foo[i] != 'a') { 
        printf("Bad data at index %d (should be a, is %c)\n", i, foo[i]);
      }
    } else if (foo[i] != 0) {
      printf("Bad data at index %d (should be 0, is %d)\n", i, foo[i]);
    }
  }

  return 0;
}

