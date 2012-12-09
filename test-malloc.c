#include <unistd.h>
#include <stdio.h>
#include "./sic.h"

int main() {
  sic_init();
  char *foo = sic_malloc(50000);
  int i;
  int base = 10000  + sic_id() * 1024;
  for (i = 0; i < 128; i++) {
    if (i % 8 == 0) {
      if (sic_id() == 0)
        foo[base + i] = 'a';
      else
        foo[base + i] = 'b';
    }
  }
  printf("foo[0]: %c\n", foo[0]);

  sic_barrier(0);
  printf("foo[10000]: %c\n", foo[10000]);
  printf("foo[11024]: %c\n", foo[11024]);

  return 0;
}

