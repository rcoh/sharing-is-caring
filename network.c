#include "network.h"

int main(int argc, char * argv[]) {
  runserver(argc, argv);
}

int runserver(int argc, char * argv[]) {
  char *replies[] = {
    "Hello World"
  };
  int listener_d = socket(PF_INET, SOCK_STREAM, 0);
  struct sockaddr_in name;
  name.sin_family = PF_INET;
  name.sin_port = (in_port_t)htons(30000);
  name.sin_addr.s_addr = htonl(INADDR_ANY);
  bind(listener_d, (struct sockaddr *) &name, sizeof(name));
  listen(listener_d, 10);
  puts("Waiting for connection . . .\n");
  while (1) {
  }

}

