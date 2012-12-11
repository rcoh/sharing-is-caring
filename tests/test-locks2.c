#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "../sic.h"

int main() {
  sic_init();
  srand(0x828);
  const size_t ARR_SIZE = 512;
  char *dat = sic_malloc(ARR_SIZE);
  int i;

  for (i = 0; i < ARR_SIZE; i++)
    dat[i] = 1;

  sic_barrier(0);

  sic_lock(0);

  printf("Acquired lock! Beginning modifications with dat: %d and id %i. Values should be 2*dat + id\n", dat[0], sic_id()+1);
  for (i = 0; i < ARR_SIZE; i++)
    dat[i] = (2*dat[i]) + (sic_id()+1);
  sic_unlock(0);

  printf("Values:\n");
  for (i = 0; i < ARR_SIZE; i++)
    printf("%d ", dat[i]);

  sic_exit();
  return 0;

}

