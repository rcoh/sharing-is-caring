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

void sic_lock(lock_id lock) {
  Transmission * tran;
  while(1) {
    tran = full_query_server(CLIENT_REQUEST_LOCK, lock);
    if(tran->code == SERVER_LOCK_NOT_ACQUIRED) {
      free(tran);
      sched_yield();
      continue;
    }
    break;
  }
  sic_debug("[CLIENT] %d acquired lock %d", sic_id(), lock);
  free(tran);
}

void sic_unlock(lock_id id) {
 int code = signal_server(CLIENT_RELEASE_LOCK, id, NO_ACK);
 if (code == SERVER_LOCK_NOT_RELEASED) {
   sic_debug("[CLIENT] Could not release lock %d because %d never acquired it", id, sic_id());
 } else {
   uint8_t msg[MSGMAX_SIZE];
   memset(msg, 0, MSGMAX_SIZE);
   int len = diff_and_cleanup(msg, sic_id(), CLIENT_LOCK_DIFF, id);
   sic_debug("[CLIENT] Sent diff to server for lock %d", id);
   send_message_to_server(msg, len, NO_ACK);
   sic_debug("[CLIENT] Released lock %d held by %d", id, sic_id());
 }
}

/** 
  Only id = 0 will actually perform a malloc. All parties will hit a barrier
  the virtual address malloced will be coordinated through the server.
*/
void *sic_malloc(size_t size) {
  phys_addr addr = 0;
  if (sic_id() == 0) {
    addr = alloc(size);
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
  sic_info("Malloc returned VA %p for PA %p", VIRT(addr), addr);
  // Everyone request shared address of last allocation.
  return addr;
}

/** Allow internal state to clean itself up **/
void sic_exit() {
  cleanup_client();
  pthread_detach(network_loop);
}

int sic_num_clients() {
  return query_server(CLIENT_REQUEST_NUM_CLIENTS, sic_id());
}


