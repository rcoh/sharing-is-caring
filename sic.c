#include "sic.h"
#include "sic-internals.h"


void sic_barrier(uint32_t id) {
  arrived_at_barrier(id);
}

void sic_init() {
  pthread_t network_loop;
  // Wait for server to acknowledge it knows about us
  wait_for_server();

  // Set up machinery for sic memory managment
  initialize_memory_manager(); 

  // Fire off a thread that listens for messages from the server
  pthread_create(&network_loop, NULL, runclient, NULL);
}

void sic_lock(lock_id id) {
  // Send packet to server
  // uint8_t result[255];
  // TODO: real protocol message
  // TODO: loop while result indicates we don't have the lock
  // send_packet(SERVER_IP, SERVER_PORT, "locked", result);
}

void *sic_malloc(size_t size) {
  // Allocate a multiple of PGSIZE bytes, aligned at a page
  void *res = memalign(PGSIZE, ROUNDUP(size, PGSIZE));
  mark_read_only(res, size);
  return res;
}


