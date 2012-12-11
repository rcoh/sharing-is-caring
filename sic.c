#include "sic.h"
#include "sic-internals.h"

pthread_t network_loop;

/** Barrier at the given barrier id until all calling
 * processes arrive
 **/
void sic_barrier(uint32_t id) {
  arrived_at_barrier(id);
}

/** Initialize the client's memory mapping and request
 * an id from the master server. Once we are ready, spin
 * up a thread that will process messages from the server.
 */
void sic_init() {
  initialize_client();
  pthread_create(&network_loop, NULL, runclient, NULL);
}

/** Spin locks by passing requests to the master for
 * the provided lock id until the master tells this
 * client that it is safe to proceed.
 *
 * NOTE: This will change the memory mapping based on
 * what the previous processor stored on the master.
 */
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
  sync_pages(tran->n_diff_info, tran->diff_info);
  free(tran);
}

/** Sends a request to release the provided lock id
 * passing along the current memory diff along with it
 */
void sic_unlock(lock_id id) {
  uint8_t msg[MSGMAX_SIZE];
  memset(msg, 0, MSGMAX_SIZE);
  int len = diff_and_cleanup(msg, sic_id(), CLIENT_LOCK_DIFF, id);
  sic_debug("[CLIENT] Sent diff to server for lock %d", id);
  int code = send_message_to_server(msg, len, NO_ACK);
  if (code == SERVER_LOCK_NOT_RELEASED) {
    sic_debug("[CLIENT] Could not release lock %d because %d never acquired it", id, sic_id());
  } else {
    sic_debug("[CLIENT] Released lock %d held by %d", id, sic_id());
  }
}

/** 
  Only id = 0 will actually perform a malloc. All parties will hit a barrier.
  The virtual address malloced will be coordinated through the server.
*/
void *sic_malloc(size_t size) {
  phys_addr addr = 0;
  if (sic_id() == 0) {
    addr = alloc(size);
    // send virt address to server  
    signal_server(CLIENT_MALLOC_ADDR, (uintptr_t)VIRT(addr), ACK_ADDRESS_RECIEVED);
  }
  // Everyone hits a barrier
  // TODO: reserved barrier numbers
  arrived_at_barrier(100);
  if (sic_id() != 0) {
    // All clients who are not id=0 request shared address of last allocation.
    virt_addr malloc_region = (virt_addr)(intptr_t) query_server(CLIENT_REQUEST_LAST_ADDR, -1);
    addr = PHYS(malloc_region);
  }
  assert(addr);
  sic_info("Malloc returned VA %p for PA %p", VIRT(addr), addr);
  return addr;
}

/** Allow internal state to clean itself up **/
void sic_exit() {
  cleanup_client();
  pthread_detach(network_loop);
}

/** Returns the number of clients (workerrs) the server is expecting before
 * it will release workers from barriers
 */
int sic_num_clients() {
  return query_server(CLIENT_REQUEST_NUM_CLIENTS, sic_id());
}

