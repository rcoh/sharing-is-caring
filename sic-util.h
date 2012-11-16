#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

typedef uint32_t client_id;
typedef uint32_t barrier_id;
typedef uint32_t lock_id;

void sic_panic(char * msg);

void sic_log(const char* msg);
void sic_log_fn(const char* fn, const char* msg);
