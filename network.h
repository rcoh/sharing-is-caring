#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char * argv[]);
int runserver(int argc, char * argv[]);

void bind_to_port(int socket, int port);
int open_listener_socket();
int say(int socket, char *s);

