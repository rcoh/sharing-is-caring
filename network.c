#include "network.h"

int main(int argc, char * argv[]) {
  runserver(argc, argv);
}

int runserver(int argc, char * argv[]) {
  char *replies[] = {
    "Hello World\n"
  };
  int listener_d = socket(PF_INET, SOCK_STREAM, 0);
  int reuse_socket = 1;
  if (setsockopt(listener_d, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse_socket, sizeof(int)) < 0)
    printf("Can't set the reuse option!!!");
  else {
    struct sockaddr_in name;
    name.sin_family = PF_INET;
    name.sin_port = (in_port_t)htons(30000);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listener_d, (struct sockaddr *) &name, sizeof(name)) < 0)
      printf("Error could not bind port: %d\n", 30000);
    else {
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
    }
  }
  return 0;
}

