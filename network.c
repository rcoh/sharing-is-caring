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

int recv_data(int socket, uint8_t* rec, int len) {
  int bytes_recv = recv(socket, rec, len, 0);
  //char * repr;
  while (bytes_recv) {
    sic_logf("recv: %d/%d", rec, bytes_recv, len);
    /*
    repr = hex_repr(rec);
    sic_logf("recv: hex_repr = %s", repr);
    free(repr);
    */
    len -= bytes_recv;
    if (rec[bytes_recv] == '\0') {
      sic_logf("Found null termination after %d bytes", bytes_recv);
      return 0;
    }
    if (bytes_recv < 0)
      return bytes_recv;
    if (len == 0)
      return 0;
    sic_logf("recv: needs more bytes, waiting for them");
    bytes_recv = recv(socket, rec + bytes_recv, len, 0);
  }
  return 0;
}

int send_message(const char* ip, int port, const uint8_t* msg, int len, uint8_t* rec) {
  sic_logf("--------------------------------------------------------------------------------");
  sic_logf("Trying to send [%x] to [%s] on port [%d] with length [%d]", msg, ip, port, len);
  int socket = open_socket(ip, port);
  int result = send(socket, msg, len, 0);
  if (result < 0)
    fprintf(stderr, "Could not send packet, well fuck\n");
  sic_logf("Trying to receive response");
  result = recv_data(socket, rec, 255);
  sic_logf("Received response from server\n");
  close(socket);
  return result;
}

int send_packet(const char* ip, int port, const uint8_t* msg, int len, uint8_t* rec) {
  int socket = open_socket(ip, port);
  sic_logf("Trying to send [%s] to [%s] on port [%d]\n", msg, ip, port);
  int result = send(socket, msg, len + 1, 0);
  if (result < 0)
    fprintf(stderr, "Could not send packet, well fuck\n");
  result = recv_data(socket, rec, 255);
  close(socket);
  return result;
}

void connect_to_server() {
  // TODO: jlynch
}
