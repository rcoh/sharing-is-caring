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

/** 
  Only id = 0 will actually perform a malloc. All parties will hit a barrier
  the virtual address malloced will be coordinated through the server.
*/
void *sic_malloc(size_t size) {
  phys_addr addr = 0;
  if (sic_id() == 0) {
    addr = alloc(ROUNDUP(size, PGSIZE));
    signal_server(CLIENT_MALLOC_ADDR, (uintptr_t)VIRT(addr), ACK_ADDRESS_RECIEVED);
    // send virt address to server  
  }
  // Everyone hits a barrier
  // TODO: reserved barrier numbers
  arrived_at_barrier(100);
  if (sic_id() != 0) {
    virt_addr malloc_region = (virt_addr)(intptr_t) query_server(CLIENT_REQUEST_LAST_ADDR, -1);
    addr = PHYS(malloc_region);
  }
  assert(addr);
  sic_info("Malloc returned virtual address %x", VIRT(addr));
  // Everyone request shared address of last allocation.
  return addr;
}

void sic_claim(void* addr, size_t len);

/** Allow internal state to clean itself up **/
void sic_exit() {
  cleanup_client();
  pthread_detach(network_loop);
}


