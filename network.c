#include "network.h"
#include "sic-util.h"

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
  int total_bytes_recv = 0;
  int last_bytes_recv = recv(socket, rec, len, 0);
  total_bytes_recv += last_bytes_recv;
  //char * repr;
  while (total_bytes_recv < len && last_bytes_recv != 0) {
    sic_debug("recv: %d bytes / %d possible", total_bytes_recv, len);
    if (rec[total_bytes_recv] == '\0') {
      sic_debug("Found null termination after %d bytes", total_bytes_recv);
      break;
    }
    if (last_bytes_recv < 0)
      return last_bytes_recv;
    if (total_bytes_recv == len)
      break;
    sic_debug("recv: needs more bytes, waiting for them");
    last_bytes_recv = recv(socket, rec + total_bytes_recv, len - total_bytes_recv, 0);
    total_bytes_recv += last_bytes_recv;
  }
  return total_bytes_recv;
}

int send_message(const char* ip, int port, const uint8_t* msg, int len, uint8_t* rec) {
  sic_debug("--------------------------------------------------------------------------------");
  char tran_string[50];
  get_transmission(tran_string, msg);
  sic_debug("Trying to send [%35s] to [%s] on port [%d] with length [%d]", tran_string, ip, port, len);
  int socket = open_socket(ip, port);
  int result = send(socket, msg, len, 0);
  if (result < 0)
    fprintf(stderr, "Could not send packet\n");
  sic_debug("Trying to receive response");
  result = recv_data(socket, rec, MSGMAX_SIZE);
  sic_debug("Received response. %d bytes", result);
  close(socket);
  return result;
}

int send_packet(const char* ip, int port, const uint8_t* msg, int len, uint8_t* rec) {
  int socket = open_socket(ip, port);
  sic_debug("Trying to send [%s] to [%s] on port [%d]\n", msg, ip, port);
  int result = send(socket, msg, len + 1, 0);
  if (result < 0)
    fprintf(stderr, "Could not send packet, well fuck\n");
  result = recv_data(socket, rec, MSGMAX_SIZE);
  close(socket);
  return result;
}

void connect_to_server() {
  // TODO: jlynch
}
