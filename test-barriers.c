#include <unistd.h>
#include <stdio.h>
#include "./sic.h"

int main() {
  sic_init();
  printf("At beginning\n");
  sic_barrier(0);
  printf("Passed through barrier 0. Sleeping for a bit.\n");
  printf("Arrived barrier 1.\n");
  
  sic_barrier(1);
  printf("Through barrier 1.\n");
  printf("Arrived barrier 2\n");
  
  sic_barrier(2);
  printf("Passed barrier 2. Done\n");
  sic_exit();
  return 0;
}

