#include "sic-server.h"


static Barrier barriers[NUM_BARRIERS];
static Lock    locks[NUM_LOCKS];
static pthread_mutex_t server_lock;
static Client  clients[MAX_CLIENTS];
static int     num_clients = NUM_CLIENTS;
static virt_addr last_malloced;

int main(int argc, char *argv[], char *evnp[]) {
  int result;
  int i;
  pthread_mutex_init(&server_lock, NULL);

  if (argc == 2) {
    num_clients = atoi(argv[1]);
    if (num_clients < 0 || num_clients >= MAX_CLIENTS)
      num_clients = NUM_CLIENTS;
   }

  // Initialize the client matrix
  for (i = 0; i < num_clients; i++) {
    clients[i].port = 0;
    clients[i].present = false;
  }

  for (i = 0; i < NUM_BARRIERS; i++) {
    barriers[i].invalid_pages = NULL;
    barriers[i].id = i;
    barriers[i].num_clients_waiting = 0;
    int j;
    for (j = 0; j < MAX_CLIENTS; j++)
      barriers[i].clients_arrived[j] = false;
  }

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
  sic_debug("Waiting for connections from %d clients ...", num_clients);
  while (1) {
    struct sockaddr_in client_addr;
    unsigned int address_size = sizeof(client_addr);
    int connect_d = accept(listener_d, (struct sockaddr*) &client_addr, &address_size);
    // Recieve the message from the client
    memset(buffer, 0, sizeof(buffer));
    recv_data(connect_d, buffer, MSGMAX_SIZE);

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
  sic_debug("Server processing: id: %d, type: %s, value: %d\n", id, get_message(code), value);
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
      last_malloced = (virt_addr)(intptr_t)value;
      return encode_message(return_msg, -1, ACK_ADDRESS_RECIEVED, value);
    case CLIENT_REQUEST_LAST_ADDR:
      sic_info("Client %d request last malloced address. Returning %d", id, last_malloced);
      // TODO: handle no last_malloced
      return encode_message(return_msg, -1, ACK_ADDRESS_RECIEVED, (value_t)(intptr_t)last_malloced);
    case CLIENT_REQUEST_NUM_CLIENTS:
      sic_info("Client %d request number of clients. Returning %d", id, num_clients);
      return encode_message(return_msg, -1, CLIENT_REQUEST_NUM_CLIENTS, num_clients);
    case CLIENT_EXIT:
      sic_info("Client %d exiting.", id);
      return encode_message(return_msg, -1, ACK_CLIENT_EXIT, destroy_client(id));
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

  int i;
  client_id new_client_id = 0;
  // Find the first available client ID
  for (i = 0; i < num_clients; i++) {
    if (clients[i].present == false) {
      clients[i].present = true;
      new_client_id = i;
      break;
    } else {
      sic_debug("WELL FUCK! %d is used", i);
    }
  }
  strncpy(clients[new_client_id].host, client_ip, sizeof(clients[new_client_id].host));
  clients[new_client_id].port = CLIENT_BASE_PORT + new_client_id;

  pthread_mutex_unlock(&server_lock);
  return new_client_id;
}

/**
 * Called by the network lookn when a client sends a "client exit" request.
 * Returns true if we can successfully remove this client from the list
 */
bool destroy_client(client_id id) {
  if (id < num_clients) {
    clients[id].present = false;
    return true;
  } else {
    return false;
  }
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

  if (b->num_clients_waiting == num_clients) {
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
    l->owner = num_clients;
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
  uint8_t resp[MSGMAX_SIZE];
  memset(resp, 0, sizeof(resp));

  sic_debug("Sending message to %s on port %d\n", clients[id].host, clients[id].port);
  send_message(clients[id].host, clients[id].port, msg, len, resp);
  decode_message(resp, &cid, &ccode, &cvalue);
  if (expected_ack && ccode != expected_ack)
    sic_panic("Did not receive correct ack from client");
  sic_debug("Got response code from client: %d", ccode);
  return ccode;
}

void broadcast_barrier_release(barrier_id id) {
  int i;
  sic_debug("[SERVER] Packaging diff for barrier %i", id);
  PageInfo * bpages = barriers[id].invalid_pages;

  if (bpages)
    print_memstat(barriers[id].invalid_pages);

  for(i = 0; i < num_clients; i++) {
    uint8_t msg[MSGMAX_SIZE];
    memset(msg, 0, sizeof(msg));
    sic_debug("Packaging current diff to send to client # %i", i);
    int len = package_pageinfo(msg, i, SERVER_RELEASE_BARRIER, id, bpages);
    sic_debug("Sending barrier release message to %s on port %d\n", clients[i].host, clients[i].port);
    send_message_to_client(i, msg, len, ACK_RELEASE_BARRIER);
  }
}


void signal_lock_acquired(client_id client, lock_id lock) {
  if (client >= num_clients)
    sic_panic("Client ID too high!");
  signal_client(client, SERVER_LOCK_ACQUIRED, lock, ACK_ACQUIRED);
}

void signal_lock_not_acquired(client_id client, lock_id lock) {
  if (client >= num_clients)
    sic_panic("Client ID too high!");
  signal_client(client, SERVER_LOCK_NOT_ACQUIRED, lock, ACK_NOT_ACQUIRED);
}


void signal_successful_release(client_id client, lock_id lock) {
  if (client >= num_clients)
    sic_panic("Client ID too high!");
  signal_client(client, SERVER_LOCK_RELEASED, lock, ACK_RELEASED);
}

void signal_invalid_release(client_id client, lock_id lock) {
  if (client >= num_clients)
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
  for (i = 0; i < num_clients; i++) {
    assert(b.clients_arrived[i] == false);
  }
}

void assert_full_barrier(Barrier b) {
  int i;
  for (i = 0; i < num_clients; i++) {
    assert(b.clients_arrived[i] == true);
  }
}

void clear_barrier(Barrier *b) {
  int i;
  for (i = 0; i < num_clients; i++) {
    b->clients_arrived[i] = false;
  }
  b->num_clients_waiting = 0;
}


