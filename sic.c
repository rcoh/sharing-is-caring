#include "sic.h"
#include "sic-internals.h"


pthread_t network_loop;
void sic_barrier(uint32_t id) {
  arrived_at_barrier(id);
}

void sic_init() {
  // Initiaizes all the things
  initialize_client();

  // Fire off a thread that listens for messages from the server
  pthread_create(&network_loop, NULL, runclient, NULL);
}

void sic_lock(lock_id id) {
  acquire_lock(id);
}

void sic_unlock(lock_id id) {
  release_lock(id);
}

void *sic_malloc(size_t size) {
  return alloc(ROUNDUP(size, PGSIZE));
}

void sic_claim(void* addr, size_t len);

/** Allow internal state to clean itself up **/
void sic_exit() {
  cleanup_client();
  pthread_detach(network_loop);
}


