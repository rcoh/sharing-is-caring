#include <unistd.h>
#include <stdio.h>
#include <math.h>

#define SIZE 2000000

int main() {
  char * primes = malloc(SIZE);
  int max_check = floor(sqrt(SIZE)) + 1;
  int begin = 2;
  int end = max_check;
  int i, j;
  for (i = begin; i < end ; i++) {
    for (j = i+i; j < SIZE; j = j + i) {
      primes[j] = 1;
    }
  }

  for (i = 0; i < SIZE; i++) {
    printf("%d: %d\n", i, primes[i]);
  }
  return 0;
}

