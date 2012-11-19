#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>


#define PGSIZE 4096
// Rounding operations (efficient when n is a power of 2)
// Round down to the nearest multiple of n
#define ROUNDDOWN(a, n)           \
({                \
  uintptr_t __a = (uintptr_t) (a);        \
  (__typeof__(a)) (__a - __a % (n));        \
})
// Round up to the nearest multiple of n
#define ROUNDUP(a, n)           \
({                \
  uintptr_t __n = (uintptr_t) (n);        \
  (__typeof__(a)) (ROUNDDOWN((uintptr_t) (a) + __n - 1, __n)); \
})

typedef int client_id;
typedef uint32_t barrier_id;
typedef uint32_t lock_id;

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 1337
#define CLIENT_BASE_PORT 1338

// Error Codes 

#define E_INVALID_BARRIER -1

void sic_panic(char * msg);

void sic_log(const char* msg);
void sic_log_fn(const char* fn, const char* msg);

void sic_logf(const char* format, ...); 
