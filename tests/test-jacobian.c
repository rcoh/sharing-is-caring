#include <unistd.h>
#include <stdio.h>
#include "../sic.h"

#define M 1024
#define N 1024
#define NUM_CLIENTS 2
float *grid;
float scratch[M][N];

void init_local() {
  int i,j;
  for (i=0; i<M; i++) {
    for (j=0; j<N; j++) {
      scratch[i][j] = 0;
    }
  }
}

int main() {
  sic_init();
  grid = sic_malloc(N*M*sizeof(float));
  init_local();
  sic_barrier(0);

  printf("Beginning with sic_id %d and grid: %p\n", sic_id(), grid);
  int length = M / NUM_CLIENTS;
  int begin = length * sic_id();
  int end = length * (sic_id()+1);
  int k,i,j;
  float sum; int count;
  printf("Starting calculation l:%d, b:%d, e:%d\n", length, begin, end);
  for (k=0; k<10000; k++) {
    for (i=begin; i<end; i++) {
      for (j=0; j<N; j++) {
        sum = 0.0f;
        count = 0;
        if (0 <= i-1) {
          sum += grid[(i-1)*N + j];
          count++;
        }
        if (i+1 < M) {
          sum += grid[(i+1)*N + j];
          count++;
        }
        if (0 <= j-1) {
          sum += grid[i*N + j-1];
          count++;
        }
        if (j+1 < N) {
          sum += grid[i*N + j+1];
          count++;
        }
        scratch[i][j] = sum / (float)count;
      }
    }

    sic_barrier(1);

    for (i=begin; i<end; i++) {
      for (j=0; j<N; j++) {
        grid[i*N + j] = scratch[i][j];
      }
    }

    sic_barrier(2);
  }
  sic_exit();
  return 0;
}

