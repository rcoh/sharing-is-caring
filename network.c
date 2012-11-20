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

int recv_data(int socket, char* rec, int len) {
  int bytes_recv = recv(socket, rec, len, 0);
  while (bytes_recv) {
    //sic_logf("received: %d\n", bytes_recv);
    len -= bytes_recv;
    if (rec[bytes_recv-1] == '\0') {
      rec[bytes_recv-1] = '\0';
      return 0;
    }
    if (bytes_recv < 0)
      return bytes_recv;
    if (len == 0)
      return 0;
    bytes_recv = recv(socket, rec + bytes_recv, len, 0);
  }
  return 0;
}

int send_message(const char* ip, int port, const void *msg, int len, char *rec) {
  int socket = open_socket(ip, port);
  sic_logf("Trying to send [%s] to [%s] on port [%d]\n", msg, ip, port);
  int result = send(socket, msg, len, 0);
  if (result < 0)
    fprintf(stderr, "Could not send packet, well fuck\n");
  result = recv_data(socket, rec, 255);
  close(socket);
  return result;
}

int send_packet(const char* ip, int port, const char* msg, char* rec) {
  int socket = open_socket(ip, port);
  sic_logf("Trying to send [%s] to [%s] on port [%d]\n", msg, ip, port);
  int result = send(socket, msg, strlen(msg) + 1, 0);
  if (result < 0)
    fprintf(stderr, "Could not send packet, well fuck\n");
  result = recv_data(socket, rec, 255);
  close(socket);
  return result;
}

void connect_to_server() {
  // TODO: jlynch
}
