#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include "../sic.h"

#define SIZE 20000

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
  int i, j;
  char *primes = sic_malloc(SIZE);
  sic_barrier(0);

  int max_check = floor(sqrt(SIZE)) + 1;
  int length = max_check / sic_num_clients();
  int begin = length * sic_id();
  if (begin == 0) {
    begin = 2;
  }
  int end = length * (sic_id()+1);

  int num_computed = 3;
  for (i = begin; i < end ; i++) {
    for (j = i+i; j < SIZE; j = j + i) {
      primes[j] = 1;
    }
    num_computed++;
    if(num_computed > 13) {
      num_computed = 0;
      sic_barrier(num_computed);
    }
  }
  sic_barrier(1);

  int num_error = 0;
  // Primes should have a 0 for primes and 1's for composite
  for (i = 0; i < SIZE; i++) {
    if(is_prime(i) && (primes[i] != 0)) {
      num_error++;
    } else if (!is_prime(i) && (primes[i] == 0)) {
      num_error++;
    }
  }
  if(num_error == 0) {
    printf("[SUCCESS] No errors primes all correct!\n");
  } else {
    printf("[ERROR] %d Prime computaiton errors!\n", num_error);
  }
  sic_exit();
  return 0;
}

