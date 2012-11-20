#ifndef SIC_UTIL_H
#define SIC_UTIL_H
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

#define CLIENT_AT_BARRIER 1
#define ACK_CLIENT_AT_BARRIER 2
#define ACK_NO_SUCH_BARRIER 3
#define SERVER_RELEASE_BARRIER 4
#define ACK_RELEASE_BARRIER 5
#define CLIENT_INIT 6
#define SERVER_INIT 7

#define ERROR_ALL 99
// Error Codes 

#define E_INVALID_BARRIER -1

void sic_panic(char * msg);

void sic_log(const char* msg);
void sic_log_fn(const char* fn, const char* msg);
void sic_logf(const char* format, ...);

int encode_message(char* msg, int id, int code, int value);
int decode_message(char* msg, int* id, int* code, int* value);

// Memdiff code 

// Length of unchanged bytes -> new byte

typedef struct {
  uint16_t length: 12;
  uint8_t new_content;
} MemDiff;

typedef struct {
  MemDiff *diffs;
  size_t num_diffs;
} RegionDiff;


RegionDiff memdiff(void *old, void *new, size_t length);

void print_diff(RegionDiff diff);

#endif
