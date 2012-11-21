#include "sic.h"
#include "sic-internals.h"


void sic_barrier(uint32_t id) {
  arrived_at_barrier(id);
}

void sic_init() {
  pthread_t network_loop;
  // Initiaizes all the things
  initialize_client();

  // Fire off a thread that listens for messages from the server
  pthread_create(&network_loop, NULL, runclient, NULL);
}

void sic_lock(lock_id id) {
  // Send packet to server
  char result[255];
  // TODO: real protocol message
  // TODO: loop while result indicates we don't have the lock
  send_packet(SERVER_IP, SERVER_PORT, "locked", result);
}

void *sic_malloc(size_t size) {
  return alloc(ROUNDUP(size, PGSIZE));
}

void sic_claim(void* addr, size_t len);

void sic_exit() {
  // Allow internal state to clean itself up
  cleanup_client();
}


