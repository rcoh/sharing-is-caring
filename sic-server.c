#include <assert.h>
#include <stdbool.h>
#include <pthread.h>
#include "sic-util.h"
#include "sic-server.h"
#include "network.h"

static Barrier barriers[MAX_BARRIERS];
static Lock    locks[NUM_LOCKS];
static pthread_mutex_t server_lock;

int main(int argc, char *argv[], char *evnp[]) {
  pthread_mutex_init(&server_lock, NULL);
  runserver(argc, argv);
  return 0;
}

void client_arrived_at_barrier(client_id client, barrier_id barrier) {
  pthread_mutex_lock(&server_lock);
  // If it's the first client to the barrier
  Barrier *b = &barriers[barrier];
  if (b->id == 0) {
    b->id = barrier;
    assert_empty_barrier(*b);
  }

  if (b->clients_arrived[client] == false) {
    b->clients_arrived[client] = true;
    b->num_clients_waiting++;
  }

  if (b->num_clients_waiting == NUM_CLIENTS) {
    assert_full_barrier(*b);
    release_clients(b);
  }
  pthread_mutex_unlock(&server_lock);
}

/** 
 * Called by main network loop when a lock request comes in. Either replies
 * indicating that the lock was acquired, or replies indicating that it was not
 * acquired and that the client should retry.
 */
void client_requests_lock(client_id client, lock_id lock) {
  pthread_mutex_lock(&server_lock);
  Lock *l = &locks[lock];
  if (l->held) {
    signal_lock_not_acquired(client, lock);
  } else {
    l->held = true;
    l->owner = client;
    l->id = lock;
    signal_lock_acquired(client, lock);
  }
  pthread_mutex_unlock(&server_lock);
}

void client_frees_lock(client_id client, lock_id lock) {
  pthread_mutex_lock(&server_lock);
  Lock *l = &locks[lock];
  if (l->held && l->owner == client) {
    l->held = false;
    l->owner = NO_OWNER;
    signal_successful_release(client, lock);
  } else {
    signal_invalid_release(client, lock);
  }
}

void release_clients(Barrier *barrier) {
  // Method only called from client_arrived, so we already have the lock
  clear_barrier(barrier);
  broadcast_barrier_release(barrier->id);
}

void broadcast_barrier_release(barrier_id id) {
  // send message "barrier: id"
  printf("WTFBBQ\n");
}

void signal_lock_acquired(client_id client, lock_id lock) {
  // TODO: jlynch
}

void signal_lock_not_acquired(client_id client, lock_id lock) {
  // TODO: jlynch
}


void signal_successful_release(client_id client, lock_id lock) {
  // TODO: jlynch
}
void signal_invalid_release(client_id client, lock_id lock) {
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


