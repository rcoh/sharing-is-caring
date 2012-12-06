#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>
#include "sic-util.h"

/*
 * Start the sic process. Wait for everyone to get here.
 */
void sic_init();
void sic_exit();
/*
 * Allocate shared memory.
 */

int sic_id();

void *sic_malloc(size_t size);

void sic_free(void *ptr);

/* Shares a just-malloced pointer with the herd. Involves an implicit barrier. */
void init_var(void *ptr);

void sic_barrier(uint32_t id); 

void sic_lock(lock_id id);

void sic_unlock(lock_id id);

void memstat();
