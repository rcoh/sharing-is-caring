#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "../sic.h"
#include <crypt.h>

#define SIZE 4096

void fill(char *arr, int len) {
  int i;
  for (i=0; i < len; i++) {
    arr[i] = i;
  }
}

void complex_computation(char *dst, char *src) {
  char * salt = "$1$828ROX";
  int i = 0;
  char * result = crypt(src, salt);
  for(i = 0; i < 100; i++)
    result = crypt(result, salt);
  strncpy(dst, result, 34);
}

int main() {
  sic_init();
  int i; char *tmp;
  char **hashes = sic_malloc(SIZE*sizeof(char*));
  for (i = 0; i < SIZE; i++) {
    tmp = sic_malloc(34);
    hashes[i] = tmp;
  }
  const char * msg = "WOOT MOFO";
  char **seeds = malloc(SIZE*sizeof(char*));
  for (i = 0; i < SIZE; i++) {
    tmp = malloc(34);
    strncpy(tmp,msg,strlen(msg));
    seeds[i] = tmp;
  }

  int length = SIZE / sic_num_clients();
  int begin = length * sic_id();
  int end = length * (sic_id()+1);

  sic_barrier(0);
  int times_through = 2;
  for (i = begin; i < end ; i++) {
      complex_computation(hashes[i], seeds[i]);
      times_through++;
      if(times_through >= 13) {
        sic_barrier(times_through);
        times_through = 3;
      }
  }
  sic_barrier(1);

  if (sic_id() == 0) {
    for (i = 0; i < SIZE; i++) {
      printf("%d: %s -> %s \n", i, seeds[i], hashes[i]);
    }
  }

  sic_exit();
  return 0;
}

