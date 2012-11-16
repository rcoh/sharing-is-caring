#include "./sic.h"
#include "./sic-internals.h"
void sic_barrier(uint32_t id) {
  arrived_at_barrier(id);
}
