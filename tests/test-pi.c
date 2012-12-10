#include "../sic.h"
#include <time.h>
#include <math.h>

const float cir_cent = .5;
long npoints = 100000000;
float rand_float() {
  return (float)rand() / RAND_MAX;
}

// Is a point (x,y) inside a r = .5 circle centered at .5, .5?
bool inside_circle(float x, float y) {
  // Is dist from center > .5?
  float dx = fabs(x - cir_cent);
  float dy = fabs(y - cir_cent);
  return sqrt(dx*dx + dy*dy) < .5;
}

int main() {
  sic_init();
  int *circle_count = sic_malloc(sic_num_clients() * sizeof(int));
  int num_to_run = npoints / sic_num_clients();
  printf("Num clients: %d", sic_num_clients());

  int i;
  srand(0x828);
  for (i = 0; i < num_to_run; i++) {
    float x = rand_float();
    float y = rand_float();
    if (inside_circle(x, y)) {
      circle_count[sic_id()] += 1;
    }
  }

  sic_barrier(0);
  
  // Compute pi from results
  if (sic_id() == 0) {
    int total_circle_count = 0;
    for (i = 0; i < sic_num_clients(); i++) {
      total_circle_count += circle_count[i];
    }
    printf("PI is: %f", 4 * total_circle_count / (float)npoints);
  }
  sic_exit();
  return 0;
}

    
