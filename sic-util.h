#ifndef SIC_UTIL_H
#define SIC_UTIL_H
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <malloc.h>
#include <sys/mman.h>
#include "sic-message.pb-c.h"


#define PGSIZE 4096
#define MSGMAX_SIZE 131072
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

  CLIENT_EXIT,
  ACK_CLIENT_EXIT,
  CLIENT_REQUEST_NUM_CLIENTS,
  CLIENT_LOCK_DIFF,
  ERROR_ALL
} message_t;

static const char * const message_names[] = {
  "No Acknowledge",
  "Client at Barrier",
  "Ack Client at Barrier",
  "Ack No such Barrier",
  "Server Release Barrier",
  "Ack Release Barrier",
  "Initialize Client",
  "Initialize Server",
  "Client Request Lock",
  "Server Lock Acquired",
  "Server Lock [not] Acquired",
  "Client Malloc Address",
  "Client Request Last Address",
  "Ack Address Received",
  "Client Release Lock",
  "Server Lock Released",
  "Server Lock [not] Released",
  "Ack Released",
  "Ack Not Released",
  "Ack Acquired",
  "Ack Not Acquired",
  "Client Exiting",
  "Ack Client Exiting",
  "Number of Clients",
  "Client Diff for Lock",
  "ERROR ALL"
};


#define SERVER_IP "18.248.7.172"
#define SERVER_PORT 1337
#define CLIENT_BASE_PORT 1338

#define SHARED_SIZE (1 << 27)

#define ERROR_ALL 99
// Error Codes 

#define E_INVALID_BARRIER -1

void sic_panic(char * msg);

void sic_log(const char* msg);
void sic_log_fn(const char* fn, const char* msg);
void sic_debug(const char* format, ...);
void sic_info(const char *fmt, ...);

int encode_message(uint8_t* msg, int id, int code, value_t value);
int decode_message(uint8_t* msg, int* id, int* code, value_t* value);

int encode_transmission(uint8_t *buf, Transmission *trans); 
Transmission* decode_transmission(uint8_t *msg); 

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

void apply_diff(void *va, RegionDiff diff, bool use_or, bool allow_writes);

RegionDiff merge_diffs(RegionDiff r1, RegionDiff r2); 
PageInfo* merge_multipage_diff(PageInfo *current, int n_diffinfo, RegionDiffProto **diff_info);

int package_pageinfo(uint8_t *msg, client_id client, int code, value_t value, PageInfo * pages);



void print_diff(RegionDiff diff);
void print_memstat(PageInfo * pages);

const char* get_message(message_t message);
void get_transmission(char * ret, const uint8_t * msg);
 
char* hex_repr(char * msg);
#endif
