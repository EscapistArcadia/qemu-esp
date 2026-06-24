#include "hw/misc/esp/sm_queue.h"

// #include "system/address-spaces.h"
// #include "system/dma.h"

#define MPMC_DEGREE 2
#define SM_ENTRY_SIZE (2 * MPMC_DEGREE)
#define SM_SEQ_SIZE 2
#define SM_COMMON_SIZE 8
#define SM_SLOT_SIZE 4
#define SM_QUEUE_WORDS (SM_COMMON_SIZE + (SM_SLOT_SIZE * SM_QUEUE_SIZE))

#define QUEUE_INVALID 0
#define QUEUE_AVAIL 1
#define QUEUE_BUSY 2
