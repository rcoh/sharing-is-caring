#include <assert.h>
#include <stdbool.h>
#include "./sic-util.h"
#include "./sic-server.h"


static Barrier barriers[MAX_BARRIERS];

int main(int argc, char *argv[], char *evnp[]) {
  return 0;
}

void client_arrived_at_barrier(client_id client, barrier_id barrier) {
  // If it's the first client to the barrier
  Barrier b = barrier[barrier];
  if (b.id == 0) {
    b.id = barrier;

    // Make sure it's empty
    for (int i = 0; i < NUM_CLIENTS; i++) {
      assert(b.clients_arrived[i] == false);
    }
  }

  if (b.clients_arrived[client] == false) {
    b.clients_arrived[client] = true;
    b.num_clients_waiting++;
  }

  if (b.num_clients_waiting == NUM_CLIENTS) {
    for (int i = 0; i < NUM_CLIENTS; i++) {
      assert(b.clients_arrived[i] == true);
    }
    release_clients(barrier);
  }
}

void release_clients(barrier_id barrier) {
}
