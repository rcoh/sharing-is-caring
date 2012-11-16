#include "network.h"
#include "sic-util.h"

int main(int argc, char * argv[]) {
  runserver(argc, argv);
}

void bind_to_port(int socket, int port) {
  struct sockaddr_in name;
  name.sin_family = PF_INET;
  name.sin_port = (in_port_t)htons(port);
  name.sin_addr.s_addr = htonl(INADDR_ANY);

  int reuse_socket = 1;
  if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse_socket, sizeof(int)) < 0)
    sic_panic("Could not set socket reuse");
  if (bind(socket, (struct sockaddr *) &name, sizeof(name)) < 0)
    sic_panic("Could not bind socket");
}

int runserver(int argc, char * argv[]) {
  sic_log("Starting server ...");
  char *replies[] = {
    "Hello World\n"
  };
  int listener_d = socket(PF_INET, SOCK_STREAM, 0);
  bind_to_port(listener_d, 30000);
  listen(listener_d, 10);
  puts("Waiting for connection . . .\n");
  while (1) {
    struct sockaddr_storage client_addr;
    unsigned int address_size = sizeof(client_addr);
    int connect_d = accept(listener_d, (struct sockaddr*) &client_addr, &address_size);
    char * msg = replies[0];
    send(connect_d, msg, strlen(msg), 0);
    close(connect_d);
  }
  return 0;
}

void connect_to_server() {
  // TODO: jlynch
}
