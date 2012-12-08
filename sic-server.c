#include "sic-server.h"

static Barrier barriers[NUM_BARRIERS];
static Lock    locks[NUM_LOCKS];
static pthread_mutex_t server_lock;
static int num_clients_connected = 0;
static Client  clients[NUM_CLIENTS];

static virt_addr last_malloced;

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
  int sid, scode;
  value_t svalue;
  sic_log("Starting server ...");
  uint8_t buffer[MSGMAX_SIZE];
  int listener_d = open_listener_socket();
  bind_to_port(listener_d, SERVER_PORT);
  listen(listener_d, 10);
  sic_logf("Waiting for connections from %d clients ...", NUM_CLIENTS);
  while (1) {
    struct sockaddr_in client_addr;
    unsigned int address_size = sizeof(client_addr);
    int connect_d = accept(listener_d, (struct sockaddr*) &client_addr, &address_size);
    // Recieve the message from the client
    memset(buffer, 0, sizeof(buffer));
    recv_data(connect_d, buffer, 255);

    // Process the message
    decode_message(buffer, &sid, &scode, &svalue);
    Transmission *trans = decode_transmission(buffer);
    uint8_t msg[MSGMAX_SIZE];
    char host[1024];
    char service[24];
    getnameinfo((struct sockaddr*) &client_addr, sizeof(client_addr),
                host, sizeof(host),
                service, sizeof(service), NI_NUMERICHOST | NI_NUMERICSERV);

    int response_length = server_dispatch(msg, host, trans);

    // Send back the response message
    send(connect_d, msg, response_length, 0);
    close(connect_d);
  }
  return 0;
}

int server_dispatch(uint8_t * return_msg, const char * client_ip, Transmission *transmission) {
  int id = transmission->id, code = transmission->code; 
  value_t value = transmission->value;
  sic_logf("Server processing: id: %d, type: %s, value: %d\n", id, get_message(code), value);
  client_id result;
  message_t rcode;
  switch(code) {
    case CLIENT_INIT:
      result = new_client(client_ip);
      sic_info("Server responding with id: %d", result);
      return encode_message(return_msg, -1, SERVER_INIT, result);
    case CLIENT_AT_BARRIER:
      sic_info("Server marking %d as at barrier %d", id, value);
      client_arrived_at_barrier((client_id) id, (barrier_id) value, 
        transmission->n_diff_info, transmission->diff_info);
      return encode_message(return_msg, -1, ACK_CLIENT_AT_BARRIER, value);
    case CLIENT_REQUEST_LOCK:
      sic_info("Server attempting to acquire lock %d for %d", value, id);
      rcode = client_requests_lock(id, value);
      return encode_message(return_msg, -1, rcode, value);
    case CLIENT_RELEASE_LOCK:
      sic_info("Server attempting to release lock %d for %d", value, id);
      rcode = client_frees_lock(id, value);
      return encode_message(return_msg, -1, rcode, value);
    case CLIENT_MALLOC_ADDR:
      sic_info("Client %d malloced address %d", value, id);
      last_malloced = (virt_addr)value;
      return encode_message(return_msg, -1, ACK_ADDRESS_RECIEVED, value);
    case CLIENT_REQUEST_LAST_ADDR:
      sic_info("Client %d request last malloced address. Returning %d", id, last_malloced);
      // TODO: handle no last_malloced
      return encode_message(return_msg, -1, ACK_ADDRESS_RECIEVED, (value_t)last_malloced);
    default:
      sic_info("Can't handle %d\n", code);
      return encode_message(return_msg, -1, ERROR_ALL, -1);
  }
  return 0;
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
int client_arrived_at_barrier(client_id client, barrier_id barrier, 
                              int n_diffinfo, RegionDiffProto **diff_info) {
  if (barrier >= NUM_BARRIERS) {
    return -E_INVALID_BARRIER;
  }

  pthread_mutex_lock(&server_lock);
  // If it's the first client to the barrier
  Barrier *b = &barriers[barrier];
  b->invalid_pages = merge_multipage_diff(b->invalid_pages, n_diffinfo, diff_info);
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
message_t client_requests_lock(client_id client, lock_id lock) {
  pthread_mutex_lock(&server_lock);
  message_t rcode;
  Lock *l = &locks[lock];
  if (l->held) {
    rcode = SERVER_LOCK_NOT_ACQUIRED;
  } else {
    l->held = true;
    l->owner = client;
    l->id = lock;
    rcode = SERVER_LOCK_ACQUIRED;
  }
  pthread_mutex_unlock(&server_lock);
  return rcode;
}

message_t client_frees_lock(client_id client, lock_id lock) {
  pthread_mutex_lock(&server_lock);
  message_t rcode;
  Lock *l = &locks[lock];
  if (l->held && l->owner == client) {
    l->held = false;
    l->owner = NO_OWNER;
    rcode = SERVER_LOCK_RELEASED;
  } else {
    rcode = SERVER_LOCK_NOT_RELEASED;
  }
  pthread_mutex_unlock(&server_lock);
  return rcode;
}

void release_clients(Barrier *barrier) {
  // Method only called from client_arrived, so we already have the lock
  clear_barrier(barrier);
  broadcast_barrier_release(barrier->id);
}

void signal_client(client_id id, message_t code, int value, message_t expected_ack) {
  uint8_t msg[MSGMAX_SIZE];
  memset(msg, 0, sizeof(msg));

  int len = encode_message(msg, -1, code, value);
  send_message_to_client(id, msg, len, expected_ack);
}

int send_message_to_client(client_id id, uint8_t *msg, int len, message_t expected_ack) {
  int cid, ccode;
  value_t cvalue;
  uint8_t resp[256];
  memset(resp, 0, sizeof(resp));

  sic_logf("Sending message to %s on port %d\n", clients[id].host, clients[id].port);
  send_message(clients[id].host, clients[id].port, msg, len, resp);
  decode_message(resp, &cid, &ccode, &cvalue);
  if (expected_ack && ccode != expected_ack)
    sic_panic("Did not receive correct ack from client");
  sic_logf("Got response code from client: %d", ccode);
  return ccode;
}

void broadcast_barrier_release(barrier_id id) {
  int i;
  sic_logf("[SERVER] Packaging diff for barrier %i", id);
  PageInfo * bpages = barriers[id].invalid_pages;

  if (bpages)
    print_memstat(barriers[id].invalid_pages);

  for(i = 0; i < NUM_CLIENTS; i++) {
    uint8_t msg[MSGMAX_SIZE];
    memset(msg, 0, sizeof(msg));
    sic_logf("Packaging current diff to send to client # %i", i);
    int len = package_pageinfo(msg, i, SERVER_RELEASE_BARRIER, id, bpages);
    sic_logf("Sending barrier release message to %s on port %d\n", clients[i].host, clients[i].port);
    send_message_to_client(i, msg, len, ACK_RELEASE_BARRIER);
  }
}


void signal_lock_acquired(client_id client, lock_id lock) {
  if (client >= NUM_CLIENTS)
    sic_panic("Client ID too high!");
  signal_client(client, SERVER_LOCK_ACQUIRED, lock, ACK_ACQUIRED);
}

void signal_lock_not_acquired(client_id client, lock_id lock) {
  if (client >= NUM_CLIENTS)
    sic_panic("Client ID too high!");
  signal_client(client, SERVER_LOCK_NOT_ACQUIRED, lock, ACK_NOT_ACQUIRED);
}


void signal_successful_release(client_id client, lock_id lock) {
  if (client >= NUM_CLIENTS)
    sic_panic("Client ID too high!");
  signal_client(client, SERVER_LOCK_RELEASED, lock, ACK_RELEASED);
}

void signal_invalid_release(client_id client, lock_id lock) {
  if (client >= NUM_CLIENTS)
    sic_panic("Client ID too high!");
  signal_client(client, SERVER_LOCK_NOT_RELEASED, lock, ACK_NOT_RELEASED);
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


