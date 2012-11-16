#include <stdint.h>
#include <stdlib.h>

/*
 * Start the sic process. Wait for everyone to get here.
 */
void sic_init();

/*
 * Allocate shared memory.
 */
void *sic_malloc(size_t size);

void sic_free(void *ptr);

/* 
 * Share ptr: take a ptr, which must have been acquired by "sic_malloc" and
 * assign it's value for all workers -- the target must be static and point to
 * the same location in all workers. Example usage:
 *
 * int **numbers;
 * if(sic_pid == 0) {
 *   numbers = sic_malloc(100); 
 *   share_ptr(&numbers, (uintptr_t)numbers);
 * }
 * */
void share_ptr(void *ptr, uintptr_t value);

void sic_barrier(uint32_t id); 
