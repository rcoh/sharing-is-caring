#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

// Server methods
void bind_to_port(int socket, int port);
int open_listener_socket();
int say(int socket, char *s);

/** Connect to the master server for the cluster. Block until complete. */
void connect_to_server();

// Client methods
int open_socket(const char* ip, int port);
int send_packet(const char* ip, int port, const char* msg, char* rec);
int recv_data(int socket, char* rec, int len);
