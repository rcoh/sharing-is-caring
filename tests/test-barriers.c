#include <unistd.h>
#include <stdio.h>
#include "../sic.h"

int main() {
  sic_init();
  printf("At beginning\n");
  char * arr = sic_malloc(sic_num_clients());
  sic_barrier(0);
  printf("Passed through barrier 0. Sleeping for a bit.\n");
  sleep(2);
  printf("Arrived barrier 1.\n");
  int i,j;
  for(i = 0; i < 50; i++) {
    for (j=0; j< 200; j++) {
      sic_barrier(i);
      arr[sic_id()] = i*j;
      printf("Through barrier %d.\n", i);
    }
  }

  sic_barrier(2);
  printf("Passed barrier 2. Done\n");
  sic_exit();
  return 0;
}

