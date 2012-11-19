#include "sic-internals.h"

client_id sic_client_id = -1;

void arrived_at_barrier(barrier_id id) {
  printf("Hitting barrier: %d\n", id);
  char resp[1024];
  printf("Sending packet.\n");
  send_packet("127.0.0.1", 30000, "hello there", resp);
  printf("%s", resp);
  // TODO: jlynch send message to server
}

void released_from_barrier(barrier_id id) {
  // TODO: jlynch call from client network code
}

void wait_for_server() {
  sic_client_id = 0;
}

int sic_id() {
  return sic_client_id;
}

void *runclient(void * args) {
  int listener_d = open_listener_socket();
  int port = CLIENT_BASE_PORT + sic_id();
  bind_to_port(listener_d, port); 
  listen(listener_d, 10);
  while (1) {
    struct sockaddr_storage server_addr;
    unsigned int address_size = sizeof(server_addr);
    int connect_d = accept(listener_d, (struct sockaddr*) &server_addr, &address_size);
    char * msg = "ack from client";
    send(connect_d, msg, strlen(msg), 0);
    close(connect_d);
  }
}
