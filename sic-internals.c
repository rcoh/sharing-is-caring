#include "sic-internals.h"

client_id sic_client_id = -1;

bool blocked;

void arrived_at_barrier(barrier_id id) {
  blocked = true;
  char msg[10];
  char resp[256];
  int sid, scode, svalue;
  sic_logf("Client %d hitting barrier: %d\n", sic_id(), id);
  encode_message(msg, sic_id(), CLIENT_AT_BARRIER, id);
  send_packet(SERVER_IP, SERVER_PORT, msg, resp);
  decode_message(resp, &sid, &scode, &svalue);
  printf("Got response code from server: %d\n", scode);
  if (scode != ACK_CLIENT_AT_BARRIER)
    sic_panic("Networking error, could not get ack from client");
  while(blocked) {
    sched_yield();
  }
}

void released_from_barrier(barrier_id id) {
  assert(!blocked);
  blocked = false;
  sic_logf("Client released from barrier %d\n", id);
}

void wait_for_server() {
  sic_client_id = 0;
  char msg[10], resp[256];
  encode_message(msg, -1, CLIENT_INIT, 0);
  send_packet(SERVER_IP, SERVER_PORT, msg, resp);
  int sid, scode, svalue;
  decode_message(resp, &sid, &scode, &svalue);
  sic_client_id = svalue;
  sic_logf("Client initialized with id: %d\n", sic_id());
}

int sic_id() {
  return sic_client_id;
}

void *runclient(void * args) {
  int sid, scode, svalue;
  int listener_d = open_listener_socket();
  int port = CLIENT_BASE_PORT + sic_id();
  sic_logf("Client %d beginning to listen on port %d\n", sic_id(), port);
  char buffer[256];
  bind_to_port(listener_d, port); 
  listen(listener_d, 10);
  while (1) {
    struct sockaddr_storage server_addr;
    unsigned int address_size = sizeof(server_addr);
    int connect_d = accept(listener_d, (struct sockaddr*) &server_addr, &address_size);

    // Receive the message from the server
    memset(buffer, 0, sizeof(buffer));
    recv_data(connect_d, buffer, 255);

    // Process the message
    decode_message(buffer, &sid, &scode, &svalue);
    char msg[10];
    dispatch(msg, sid, scode, svalue);

    // Send back the response message
    send(connect_d, msg, strlen(msg), 0);
    close(connect_d);
  }
}

void dispatch(char* return_msg, int id, int code, int value) {
  sic_logf("Client processing: %d %d %d\n", id, code, value);
  switch (code) {
    case SERVER_RELEASE_BARRIER:
      encode_message(return_msg, sic_id(), ACK_RELEASE_BARRIER, value);
      released_from_barrier((barrier_id) value);
      break;
    default:
      sic_logf("Can't handle code: %d\n", code);
  }
}
