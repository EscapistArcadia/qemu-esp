#include "hw/misc/esp/sm_queue.h"

#include "hw/misc/esp/helper/dma.h"

#define MPMC_DEGREE 2
#define SM_ENTRY_SIZE (2 * MPMC_DEGREE)
#define SM_SEQ_SIZE 2
#define SM_COMMON_SIZE 8
#define SM_SLOT_SIZE 4
#define SM_QUEUE_WORDS (SM_COMMON_SIZE + (SM_SLOT_SIZE * SM_QUEUE_SIZE))

#define QUEUE_INVALID 0
#define QUEUE_AVAIL 1
#define QUEUE_BUSY 2

/* TODO: Implement the sm_queue_can_push function and sm_queue_push function */

int sm_queue_can_pop(uint64_t q, uint64_t *value) {
    uint64_t tail = dma_read(q, offsetof(sm_queue_t, tail), uint64_t),
             seq = dma_read(q, offsetof(sm_queue_t, slot[tail % SM_QUEUE_SIZE]) + offsetof(sm_queue_slot_t, seq), uint64_t);
    if (seq != (tail + 1)) {
        return false;
    }
    *value = dma_read(q, offsetof(sm_queue_t, slot[tail % SM_QUEUE_SIZE]) + offsetof(sm_queue_slot_t, entry), uint64_t);
    return true;
}

uint64_t sm_queue_pop(uint64_t q) {
    uint64_t tail = dma_read(q, offsetof(sm_queue_t, tail), uint64_t);
    size_t slot_offset = offsetof(sm_queue_t, slot) +
                         (tail % SM_QUEUE_SIZE) * sizeof(sm_queue_slot_t);
    uint64_t entry = dma_read(q, slot_offset +
                             offsetof(sm_queue_slot_t, entry), uint64_t);

    dma_write(q, slot_offset + offsetof(sm_queue_slot_t, seq), tail + SM_QUEUE_SIZE, uint64_t);
    dma_write(q, offsetof(sm_queue_t, tail), tail + 1, uint64_t);
    return entry;
}
