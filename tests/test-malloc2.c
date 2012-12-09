#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "../sic.h"

int main() {
  sic_init();
  srand(0x828);
  const size_t ARR_SIZE = 512;
  const int num_clients = 2;
  const int CHANGE_PROB = 20;
  char *dat = sic_malloc(ARR_SIZE);
  char *ref = malloc(ARR_SIZE);
  int i;
  for (i = 0; i < ARR_SIZE; i++) {
    ref[i] = 0;
    if (rand() % CHANGE_PROB == 0) {
      int data = rand() % num_clients + 1;  
      ref[i] = data;
      if (sic_id() == data - 1) {
        dat[i] = data;
      }
    }
  }

  sic_barrier(0);
  int memory_errors = 0;
  for (i = 0; i < ARR_SIZE; i++) {
    if (dat[i] != ref[i]) {
      memory_errors++;
      printf("Memory Error: index %d should have been %x but was %x\n", i, ref[i], dat[i]);
    }
  }


  if(memory_errors == 0) 
    printf("No memory errors!\n");

  return 0;
}

