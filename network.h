#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

//int main(int argc, char * argv[]);
int runserver(int argc, char * argv[]);

// Server methods
void bind_to_port(int socket, int port);
int open_listener_socket();
int say(int socket, char *s);

/** Connect to the master server for the cluster. Block until complete. */
void connect_to_server();

// Client methods
int open_socket(const char* ip, int port);
int send_packet(const char* ip, int port, const char* msg, char* rec);
