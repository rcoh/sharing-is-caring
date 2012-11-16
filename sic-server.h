#ifndef SIC_SERVER
#define SIC_SERVER
#include <stdbool.h>
#include "./sic-util.h"

#define MAX_BARRIERS 1000
#define NUM_CLIENTS 2

typedef struct {
  barrier_id id;
  bool clients_arrived[NUM_CLIENTS];
  uint32_t num_clients_waiting;
} Barrier;
/** Called by network code when the server gets new client. */
void client_arrived_at_barrier(client_id client, barrier_id barrier);


/** 
 * ----------------------------------------
 *             Private methods 
 * ----------------------------------------
 */

/** 
 * Calls network code broadcasting to all clients, and marks the barrier as 
 * cleared. 
 */

void release_clients(Barrier *barrier);

void broadcast_barrier_release(barrier_id id);

void assert_empty_barrier(Barrier b);

void assert_full_barrier(Barrier b);

void clear_barrier(Barrier *b);
#endif
