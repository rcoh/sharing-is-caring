#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include "../sic.h"

#define SIZE 2000000

bool is_prime(int num) {
  int max_check = floor(sqrt(num)) + 1;
  int i;
  for(i = 2; i < max_check; i++) {
    if (num % i == 0)
      return false;
  }
  return true;
}

int main() {
  sic_init();
  char *primes = sic_malloc(SIZE);
  int max_check = floor(sqrt(SIZE)) + 1;
  int length = max_check / sic_num_clients();
  int begin = length * sic_id();
  if (begin == 0) {
    begin = 2;
  }
  int end = length * (sic_id()+1);

  sic_barrier(0);
  int i, j;
  for (i = begin; i < end ; i++) {
    for (j = i+i; j < SIZE; j = j + i) {
      primes[j] = 1;
    }
  }
  sic_barrier(1);

  bool has_error = false;
  // Primes should have a 0 for primes and 1's for composite
  for (i = 0; i < SIZE; i++) {
    if(is_prime(i) && (primes[i] != 0)) {
      printf("ERROR: got %i wrong. Primes has %d but value is prime.\n", i, primes[i]);
      has_error = true;
    } else if (!is_prime(i) && (primes[i] == 0)) {
      printf("ERROR: got %i wrong. Primes has %d but value is not prime\n", i, primes[i]);
      has_error = true;
    } else {
      if (sic_id() == 0)
        printf("CORRECT: got %i correct\n", i);
    }
  }
  if(!has_error) {
    printf("No errors primes all correct!\n");
  }
  sic_exit();
  return 0;
}

