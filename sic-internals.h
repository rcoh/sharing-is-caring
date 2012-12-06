#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>
#include <assert.h>
#include <sched.h>
#include <signal.h>
#include <string.h>
#include <malloc.h>


#include <sys/mman.h>

#include "sic-util.h"
#include "network.h"


/** Init / Exit Methods **/
void initialize_client();
void cleanup_client();

/** Barrier Methods **/

/** Client arrived at barrier. Blocks until barrier is clear. */
void arrived_at_barrier(barrier_id barrier);

/** Client released from barrier. Should be called from network code. */
void released_from_barrier(barrier_id barrier);


/** Locking Methods **/
void acquire_lock(lock_id lock);

void release_lock(lock_id lock);

/** Wait for server to come up, and conect to it. Block until that is done. */
void wait_for_server();

/** Run the client main loop waiting for network traffic from the server */
void * runclient(void * args);

// TODO: refactor to be nicer
int sic_id();

int dispatch(uint8_t* msg, int id, int code, value_t value);

void mark_read_only(void *start, size_t length);

void initialize_memory_manager();

void to_proto(RegionDiff r, RegionDiffProto *rp);

virt_addr VIRT(phys_addr a);
phys_addr PHYS(virt_addr a);

value_t query_server(message_t code, value_t value);

/** 
 * Clone a page within the shared virtual address space into the local address
 * space and memcpy the old page contents there. 
 *
 * Remove write protections on the old page. 
 *
 * Return the address of the twin.
 */
void *twin_page(void * va);

/** 
 * Register a just-twinned page in the list of currently invalid pages.
 */

void register_page(void *old_va, void *new_va);

/** Logs the current state of affairs **/
void memstat();

/** Actually does a malloc on our shared memory space **/
void * alloc(size_t len);

/* Wrapper around memdiff */
RegionDiff diff_for_page(PageInfo *p);

int diff_and_cleanup(uint8_t *msg, client_id client, int code, value_t value);

int send_message_to_server(uint8_t *msg, int len, message_t expected_ack); 
int signal_server(message_t code, value_t value, message_t expected_ack);
