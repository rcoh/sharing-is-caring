#include <assert.h>
#include <stdbool.h>
#include <pthread.h>
#include "./sic-util.h"
#include "./sic-server.h"


static Barrier barriers[MAX_BARRIERS];
static pthread_mutex_t server_lock;

int main(int argc, char *argv[], char *evnp[]) {
  pthread_mutex_init(&server_lock, NULL);
  return 0;
}

void client_arrived_at_barrier(client_id client, barrier_id barrier) {
  pthread_mutex_lock(&server_lock);
  // If it's the first client to the barrier
  Barrier b = barriers[barrier];
  if (b.id == 0) {
    b.id = barrier;
    assert_empty_barrier(b);
  }

  if (b.clients_arrived[client] == false) {
    b.clients_arrived[client] = true;
    b.num_clients_waiting++;
  }

  if (b.num_clients_waiting == NUM_CLIENTS) {
    assert_full_barrier(b);
    release_clients(&b);
  }
  pthread_mutex_unlock(&server_lock);
}

void release_clients(Barrier *barrier) {
  // Method only called from client_arrived, so we already have the lock
  clear_barrier(barrier);
  broadcast_barrier_release(barrier->id);
}

void broadcast_barrier_release(barrier_id id) {
  // TODO: jlynch
}


/** 
 * ----------------------------------------
 *             Private methods 
 * ----------------------------------------
 */
void assert_empty_barrier(Barrier b) {
  // Make sure it's empty
  for (int i = 0; i < NUM_CLIENTS; i++) {
    assert(b.clients_arrived[i] == false);
  }
}

void assert_full_barrier(Barrier b) {
  for (int i = 0; i < NUM_CLIENTS; i++) {
    assert(b.clients_arrived[i] == true);
  }
}

void clear_barrier(Barrier *b) {
  for (int i = 0; i < NUM_CLIENTS; i++) {
    b->clients_arrived[i] = false;
  }
  b->num_clients_waiting = 0;
}


