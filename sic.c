#include "sic.h"
#include "sic-internals.h"

void sic_barrier(uint32_t id) {
  arrived_at_barrier(id);
}

void sic_init() {
  pthread_t network_loop;
  // Wait for server to acknowledge it knows about us
  wait_for_server();
  // Fire off a thread that listens for messages from the server
  pthread_create(&network_loop, NULL, runclient, NULL);
}
