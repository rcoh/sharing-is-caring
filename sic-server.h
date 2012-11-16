#include "./sic-types.h"

/** Called by network code when the server gets new client. */
void client_arrived_at_barrier(client_id client, barrier_id barrier);

/** Calls network code broadcasting to all clients. */
void release_clients(barrier_id barrier);
