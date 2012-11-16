#include "sic-internals.h"

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
}

