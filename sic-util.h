#ifndef SIC_UTIL_H
#define SIC_UTIL_H
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#include "sic-message.pb-c.h"


#define PGSIZE 4096
#define MSGMAX_SIZE 1024 
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
typedef uint32_t DiffGranularity;
typedef int64_t  value_t;

typedef void * virt_addr;
typedef void * phys_addr;
typedef enum {
  NO_ACK,
  CLIENT_AT_BARRIER,
  ACK_CLIENT_AT_BARRIER,
  ACK_NO_SUCH_BARRIER,
  SERVER_RELEASE_BARRIER,
  ACK_RELEASE_BARRIER,
  CLIENT_INIT,
  SERVER_INIT,

  CLIENT_REQUEST_LOCK,
  SERVER_LOCK_ACQUIRED,
  SERVER_LOCK_NOT_ACQUIRED,

  CLIENT_MALLOC_ADDR,
  CLIENT_REQUEST_LAST_ADDR,
  ACK_ADDRESS_RECIEVED,

  CLIENT_RELEASE_LOCK,
  SERVER_LOCK_RELEASED,
  SERVER_LOCK_NOT_RELEASED,
  ACK_RELEASED,
  ACK_NOT_RELEASED,
  ACK_ACQUIRED,
  ACK_NOT_ACQUIRED,
  ERROR_ALL
} message_t;

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 1337
#define CLIENT_BASE_PORT 1338

#define SHARED_SIZE (1 << 20)

#define ERROR_ALL 99
// Error Codes 

#define E_INVALID_BARRIER -1

void sic_panic(char * msg);

void sic_log(const char* msg);
void sic_log_fn(const char* fn, const char* msg);
void sic_logf(const char* format, ...);

int encode_message(uint8_t* msg, int id, int code, value_t value);
int decode_message(uint8_t* msg, int* id, int* code, value_t* value);

int encode_transmission(uint8_t *buf, Transmission *trans); 
Transmission *decode_transmission(uint8_t *msg); 

// Memdiff code 

typedef struct {
  uint16_t length: 12;
  uint8_t new_content;
} MemDiff;

typedef struct {
  DiffSegment *diffs;
  size_t num_diffs;
} RegionDiff;

typedef struct __PageInfo {
  virt_addr real_page_addr;
  phys_addr twinned_page_addr;
  RegionDiff diff;
  struct __PageInfo *next;
} PageInfo;

void from_proto(RegionDiff *r, RegionDiffProto *rp);
void to_proto(RegionDiff r, RegionDiffProto *rp); 

RegionDiff memdiff(void *old, void *new, size_t length);

void applydiff(void *va, RegionDiff diff);

void print_diff(RegionDiff diff);

const char* get_message(message_t message);

char* hex_repr(char * msg);
#endif
