#include "network.h"
#include "sic-util.h"

/*
int main(int argc, char * argv[]) {
  runserver(argc, argv);
}
*/

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

int open_listener_socket() {
  int s = socket(PF_INET, SOCK_STREAM, 0);
  if (s < -1)
    sic_panic("Could not open socket");
  return s;
}

int open_socket(const char* ip, int port) {
  int s = socket(PF_INET, SOCK_STREAM, 0);
  struct sockaddr_in si;
  memset(&si, 0, sizeof(si));
  si.sin_family = PF_INET;
  si.sin_addr.s_addr = inet_addr(ip);
  si.sin_port = htons(port);
  connect(s, (struct sockaddr *) &si, sizeof(si));
  return s;
}

int send_packet(const char* ip, int port, const char* msg, char* rec) {
  int socket = open_socket(ip, port);
  int result = send(socket, msg, strlen(msg), 0);
  if (result < 0)
    fprintf(stderr, "Could not send packet, well fuck\n");
  int bytes_recv = recv(socket, rec, 255, 0);
  while (bytes_recv) {
    if (bytes_recv < 0)
      break;
    rec[bytes_recv] = '\0';
    bytes_recv = recv(socket, rec, 255, 0);
  }
  close(socket);
  return result;
}

int runserver(int argc, char * argv[]) {
  sic_log("Starting server ...");
  char *replies[] = {
    "Hello World\n"
  };
  int listener_d = open_listener_socket();
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
