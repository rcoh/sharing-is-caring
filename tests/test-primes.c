#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include "../sic.h"

#define SIZE 10000000

bool is_prime(int num) {
  int max_check = floor(sqrt(num)) + 1;
  int i;
  for(i = 2; i < max_check; i++) {
    if (num % i == 0)
      return false;
  }
  return true;
}

// Take all the stack overflow code
bool is_palindrome(int num) {
  int n = num;
  int rev = 0;
  int dig;
  while (num > 0) {
    dig = num % 10;
    rev = rev * 10 + dig;
    num = num / 10;
  }
  return n == rev;
}
// And stop

int main() {
  sic_init();
  int i;
  char *primes = sic_malloc(SIZE);
  sic_barrier(0);

  int length = SIZE / sic_num_clients();
  int begin = length * sic_id();
  if (begin == 0) {
    begin = 2;
  }
  int end = length * (sic_id()+1);

  for (i = begin; i < end ; i++) {
    if(is_prime(i) && is_palindrome(i)) {
      primes[i] = 1;
    }
  }
  sic_barrier(1);

  if (sic_id() == 0) {
    printf("Found: ");
    for (i = 0; i < SIZE; i++) {
      if (primes[i] == 1) {
        printf("%d, ", i); 
      }
    }
    printf("\n");
  }
  sic_exit();
  return 0;
}

