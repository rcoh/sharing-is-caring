#include <unistd.h>
#include <stdio.h>
#include "./sic.h"

int main() {
  sic_init();
  char *foo = sic_malloc(128);
  foo[0] = 'a';
  printf("foo[0]: %c\n", foo[0]);

  return 0;
}

