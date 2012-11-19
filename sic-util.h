#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

typedef int client_id;
typedef uint32_t barrier_id;
typedef uint32_t lock_id;

#define SERVER "127.0.0.1"
#define SERVER_PORT 1337
#define CLIENT_BASE_PORT 1338

void sic_panic(char * msg);

void sic_log(const char* msg);
void sic_log_fn(const char* fn, const char* msg);
