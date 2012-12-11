#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>
#include "sic-util.h"

// Must be called at start of user-space programs
void sic_init();

// Must be called at end of user-space programs
void sic_exit();

// Each client is assigned a unique id -- find the id of the currently running
// code 
int sic_id();

// Returns the number of sic-clients running on the system. Constant throughout
// an invocation of a given sic-program.
int sic_num_clients();

// Allocate a shared chunk of memory. The memory is automatically zeroed, but
// is not backed by physical memory until it's used. All clients must issue a
// call malloc -- eg. the following code is not valid:
// if (sic_id() == 0) { sic_malloc(100); }
//
// sic_malloc involves an implicit barrier under the hood to facilitate pointer
// sharing.
void *sic_malloc(size_t size);

// free shared memory. Currently a NOP.
void sic_free(void *ptr);

// Blocks until all calling processes arrive. All barriers must have unique
// identifiers.
void sic_barrier(uint32_t id); 

// Mutual-exclusion object. All locks must have unique ids.
void sic_lock(lock_id id);

// Release mutex of given id.
void sic_unlock(lock_id id);

// Prints the current state of memory including diffs from current global state.
void memstat();

