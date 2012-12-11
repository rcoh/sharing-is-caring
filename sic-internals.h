#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>
#include <assert.h>
#include <sched.h>
#include <signal.h>
#include <string.h>
#include <sys/mman.h>

#include "sic-util.h"
#include "network.h"


/** Init / Exit Methods **/
void initialize_client();
void wait_for_server();
void cleanup_client();

/** Memory syncronization methods **/
void sync_pages(int n_diffinfo, RegionDiffProto** diff_info);

/** Barrier Methods **/
void arrived_at_barrier(barrier_id barrier);
void released_from_barrier(barrier_id barrieri, int n_diffinfo, RegionDiffProto** diff_info);

/** Locking Methods **/
void acquire_lock(lock_id lock);
void release_lock(lock_id lock);

/** Run the client main loop waiting for network traffic from the server
 * dispatching callls to the correct functions based on the transmission
 */
void * runclient(void * args);
int dispatch(uint8_t* msg, Transmission* transmission);

/**Returns the current client's id number **/
int sic_id();


/**
 * Generic Memory Methods
 */
void initialize_memory_manager();
void mark_read_only(void *start, size_t length);


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

/**
 * Actually does a malloc on our shared memory space
 */
void * alloc(size_t len);

/**
 * Diffing Methods
 */
RegionDiff diff_for_page(PageInfo *p);
int diff_and_cleanup(uint8_t *msg, client_id client, int code, value_t value);


/**
 * Communication methods to talk to the server
 */
int send_message_to_server(uint8_t *msg, int len, message_t expected_ack); 
int signal_server(message_t code, value_t value, message_t expected_ack);
value_t query_server(message_t code, value_t value);
Transmission* full_query_server(message_t code, value_t value);

/**
 * Convenience Methods
 */
void memstat();
void to_proto(RegionDiff r, RegionDiffProto *rp);
virt_addr VIRT(phys_addr a);
phys_addr PHYS(virt_addr a);




