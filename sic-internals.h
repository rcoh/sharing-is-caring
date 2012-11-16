#include <stdint.h>
#include "./sic-types.h"
/** Begin Client Code */
/** Client arrived at barrier. Blocks until barrier is clear. */
void arrived_at_barrier(barrier_id barrier);

/** Client released from barrier. Should be called from network code. */
void released_from_barrier(barrier_id barrier);

