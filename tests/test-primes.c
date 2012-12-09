#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include "../sic.h"

#define SIZE 1000000

int main() {
  sic_init();
  char *primes = sic_malloc(SIZE);
  int max_check = floor(sqrt(SIZE));
  int length = max_check / sic_num_clients();
  int begin = length * sic_id();
  if (begin == 0) {
    begin = 2;
  }
  int end = length * (sic_id()+1);

  sic_barrier(0);
  int i, j;
  for (i = begin; i < end ; i++) {
    for (j = i; j < SIZE; j = j + i) {
      primes[j] = 1;
    }
  }
  sic_barrier(1);

  for (i = 0; i < 100; i++) {
    printf("%d: %d\n", i, primes[i]);
  }
  sic_exit();
  return 0;
}

