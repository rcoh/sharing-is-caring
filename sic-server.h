#ifndef SIC_SERVER_H
#define SIC_SERVER_H
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>

#include "sic-util.h"
#include "network.h"

#define NUM_BARRIERS 1000
#define NUM_LOCKS    1000
#define MAX_CLIENTS  100
#define NUM_CLIENTS  3

typedef struct {
  barrier_id id;
  bool clients_arrived[MAX_CLIENTS];
  uint32_t num_clients_waiting;
  PageInfo *invalid_pages;
} Barrier;

typedef struct {
  lock_id id;
  client_id owner;
  bool held;
  PageInfo *invalid_pages;
} Lock;

typedef struct {
  char host[1024];
  int port;
  bool present;
} Client;



void * runserver(void * args);
int server_dispatch(uint8_t * return_msg, const char * client_ip, Transmission *transmission);

/** Called by network code when the server gets new client. */
int client_arrived_at_barrier(client_id client, barrier_id barrier, 
                              int n_diffinfo, RegionDiffProto **diff_info);

int client_arrived_at_lock(client_id client, lock_id lock,
                           int n_diffinfo, RegionDiffProto **diff_info);

/** Called by network code when client requests a lock. */
message_t client_requests_lock(client_id client, lock_id lock);

/** Called by network code when client frees a lock. */
message_t client_frees_lock(client_id client, lock_id lock);


/** 
 * ----------------------------------------
 *             Private methods 
 * ----------------------------------------
 */

/** 
 * Calls network code broadcasting to all clients, and marks the barrier as 
 * cleared. 
 */

/** client setup signals */
client_id new_client(const char * inet_addr);

/** client destroy signals */
bool destroy_client(client_id id);

/** Barrier signals */
void release_clients(Barrier *barrier);
void broadcast_barrier_release(barrier_id id);

/** Lock signals */

void signal_lock_acquired(client_id client, lock_id lock);
void signal_lock_not_acquired(client_id client, lock_id lock);

void signal_successful_release(client_id client, lock_id lock);
void signal_invalid_release(client_id client, lock_id lock);


void assert_empty_barrier(Barrier b);

void assert_full_barrier(Barrier b);

void clear_barrier(Barrier *b);

/** Networking Convenience Methods */

void signal_client(client_id id, message_t code, int value, message_t expected_ack);
int send_message_to_client(client_id id, uint8_t *msg, int len, message_t expected_ack);

#endif
