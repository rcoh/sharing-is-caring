#include "sic-server.h"

static Barrier barriers[NUM_BARRIERS];
static Lock    locks[NUM_LOCKS];
static pthread_mutex_t server_lock;
static int num_clients_connected = 0;
static Client  clients[NUM_CLIENTS];

int main(int argc, char *argv[], char *evnp[]) {
  int result;
  pthread_mutex_init(&server_lock, NULL);
  // Start the main server loop
  pthread_t network_loop;
  pthread_create(&network_loop, NULL, runserver, NULL);
  pthread_join(network_loop, (void*) &result);
  return (int)result;
}

void * runserver(void * args) {
  int sid, scode, svalue;
  sic_log("Starting server ...");
  char buffer[256];
  int listener_d = open_listener_socket();
  bind_to_port(listener_d, SERVER_PORT);
  listen(listener_d, 10);
  printf("Waiting for connection ...\n");
  while (1) {
    memset(buffer, 0, sizeof(buffer));
    struct sockaddr_in client_addr;
    unsigned int address_size = sizeof(client_addr);
    int connect_d = accept(listener_d, (struct sockaddr*) &client_addr, &address_size);

    // Recieve the message from the client
    memset(buffer, 0, sizeof(buffer));
    recv_data(connect_d, buffer, 255);

    // Process the message
    decode_message(buffer, &sid, &scode, &svalue);
    char msg[10];
    char host[1024];
    char service[24];
    getnameinfo((struct sockaddr*) &client_addr, sizeof(client_addr),
                host, sizeof(host),
                service, sizeof(service), NI_NUMERICHOST | NI_NUMERICSERV);

    server_dispatch(msg, host, sid, scode, svalue);

    // Send back the response message
    send(connect_d, msg, strlen(msg), 0);
    close(connect_d);
  }
  return 0;
}

void server_dispatch(char * return_msg, const char * client_ip, int id, int code, int value) {
  sic_logf("Server processing: %d, %d, %d\n", id, code, value);
  client_id result;
  switch(code) {
    case CLIENT_INIT:
      result = new_client(client_ip);
      sic_logf("Server responding with id: %d\n", result);
      encode_message(return_msg, -1, SERVER_INIT, result);
      break;
    case CLIENT_AT_BARRIER:
      sic_logf("Server marking %d as at barrier %d\n", id, value);
      client_arrived_at_barrier((client_id) id, (barrier_id) value);
      encode_message(return_msg, -1, ACK_CLIENT_AT_BARRIER, value);
      break;
    default:
      encode_message(return_msg, -1, ERROR_ALL, -1);
      sic_logf("Can't handle %d\n", code);
  }
}

/** 
 * Called by the network loop when a client sends a "new client" request.
 * Returns the id of the new client. The server should reply with the clients
 * id, and note the (ip, port) -> client id mapping.
 */
client_id new_client(const char * client_ip) {
  pthread_mutex_lock(&server_lock);
  client_id new_client_id = num_clients_connected++;
  strncpy(clients[new_client_id].host, client_ip, sizeof(clients[new_client_id].host));
  clients[new_client_id].port = CLIENT_BASE_PORT + new_client_id;
  pthread_mutex_unlock(&server_lock);
  return new_client_id;
}

/** 
 * Returns 0 if we recognize the client arriving at the barrier. Error codes:
 * -E_INVALID_BARRIER if the barrier id is bogus.
 */
int client_arrived_at_barrier(client_id client, barrier_id barrier) {
  if (barrier >= NUM_BARRIERS) {
    return -E_INVALID_BARRIER;
  }

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
  return 0;
}

/** 
 * Called by main network loop when a lock request comes in. Either replies
 * indicating that the lock was acquired, or replies indicating that it was not
 * acquired and that the client should retry.
 *
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
  int i;
  char msg[10];
  char resp[256];
  int cis, ccode, cvalue;
  for(i = 0; i < NUM_CLIENTS; i++) {
    memset(msg, 0, sizeof(msg));
    memset(resp, 0, sizeof(resp));
    sic_logf("Sending barrier release message to %s on port %d\n", clients[i].host, clients[i].port);
    encode_message(msg, -1, SERVER_RELEASE_BARRIER, id);
    send_packet(clients[i].host, clients[i].port, msg, resp);
    decode_message(resp, &cis, &ccode, &cvalue);
    if (ccode != ACK_RELEASE_BARRIER)
      sic_panic("Did not hear back from client");
  }
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
  int i;
  for (i = 0; i < NUM_CLIENTS; i++) {
    assert(b.clients_arrived[i] == false);
  }
}

void assert_full_barrier(Barrier b) {
  int i;
  for (i = 0; i < NUM_CLIENTS; i++) {
    assert(b.clients_arrived[i] == true);
  }
}

void clear_barrier(Barrier *b) {
  int i;
  for (i = 0; i < NUM_CLIENTS; i++) {
    b->clients_arrived[i] = false;
  }
  b->num_clients_waiting = 0;
}


