#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "../sic.h"
#include <crypt.h>

#define SIZE 512

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

// Generate a bunch of strings
// Too lazy to figure this out on my own, taken from Stack Overflow
int inc(char *c){
    if(c[0]==0) return 0;
    if(c[0]=='z'){
        c[0]='a';
        return inc(c+sizeof(char));
    }
    c[0]++;
    return 1;
}

void generate(char **dst, int limit){
    int n = 3;
    int i,j,k;
    k = 0;
    char *c = malloc((n+1)*sizeof(char));
    for(i=1;i<=n;i++){
        for(j=0;j<i;j++) c[j]='a';
        c[i]=0;
        do {
            dst[k] = malloc(34);
            strcpy(dst[k],c);
            k++;
            if (k >= limit) {
              free(c);
              return;
            }
        } while(inc(c));
    }
    free(c);
}
// End taken from stack overflow

int main() {
  sic_init();
  int i; char *tmp;
  char **hashes = sic_malloc(SIZE*sizeof(char*));
  for (i = 0; i < SIZE; i++) {
    tmp = sic_malloc(34);
    hashes[i] = tmp;
  }
  char **seeds = malloc(SIZE*sizeof(char*));
  generate(seeds, SIZE);

  int length = SIZE / sic_num_clients();
  int begin = length * sic_id();
  int end = length * (sic_id()+1);

  sic_barrier(0);
  int times_through = 2;
  for (i = begin; i < end ; i++) {
      complex_computation(hashes[i], seeds[i]);
      times_through++;
      /*if(times_through >= 20) {
        sic_barrier(times_through);
        times_through = 2;
      }*/
  }
  sic_barrier(1);

  for (i = 0; i < SIZE; i++) {
    printf("%d @ PA[0x%p]: %s -> %s \n", i, &hashes[i], seeds[i], hashes[i]);
  }

  sic_exit();
  return 0;
}

