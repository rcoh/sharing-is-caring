#include <unistd.h>
#include <stdio.h>
#include "./sic.h"

int main() {
  sic_init();
  char *foo = sic_malloc(128);
  int i;
  for (i = 0; i < 128; i++) {
    if (i % 8 == 0) {
      foo[i] = 'a';
    }
  }
  printf("foo[0]: %c\n", foo[0]);

  sic_barrier(0);

  return 0;
}

