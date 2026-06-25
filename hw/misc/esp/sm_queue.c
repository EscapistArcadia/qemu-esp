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

int sm_queue_can_push(uint64_t q) {
    int i;
    uint64_t head, head_double_check, tail, head_slot, seq;
    uint64_t q_init, q_next;
    bool retry;

    q_init = q;
    if (q == 0) {
        return false;
    }

    for (i = 0; i < MPMC_DEGREE && q != 0; ++i) {
        head = dma_read(q, offsetof(sm_queue_t, head), uint64_t);
        tail = dma_read(q, offsetof(sm_queue_t, tail), uint64_t);
        if ((head - tail) >= SM_QUEUE_SIZE) {
            return false;
        }

        head_slot = head % SM_QUEUE_SIZE;

        seq = dma_read(q, offsetof(sm_queue_t, slot[head_slot]) + offsetof(sm_queue_slot_t, seq), uint64_t);
        if (seq != head) {
            return false;
        }

        q_next = dma_read(q, offsetof(sm_queue_t, next_queue_ptr), uint64_t);
        if (q_next == 0) {
            break;
        }
        q = q_next;
    }

    q = q_init;
    for (i = 0; i < MPMC_DEGREE; ++i) {
        if (q == 0) {
            break;
        }

        retry = false;
        while (true) {
            head = dma_read(q, offsetof(sm_queue_t, head), uint64_t);
            head_slot = head % SM_QUEUE_SIZE;
            if (retry) {
                seq = dma_read(q, offsetof(sm_queue_t, slot[head_slot]) + offsetof(sm_queue_slot_t, seq), uint64_t);
                if (seq != head) {
                    continue;
                }
            }

            /* reread, and check if someone else has updated it inside memory */
            head_double_check = dma_read(q, offsetof(sm_queue_t, head), uint64_t);
            if (head != head_double_check) {
                head = head_double_check;
                retry = true;
                continue;
            }
            dma_write(q, offsetof(sm_queue_t, head), head + 1, uint64_t);
            break;
        }

        q_next = dma_read(q, offsetof(sm_queue_t, next_queue_ptr), uint64_t);
        if (q_next == 0) {
            break;
        }
        q = q_next;
    }
    return true;
}

void sm_queue_push(uint64_t q, uint64_t entry) {
    int i;
    uint64_t head, head_slot;
    volatile uint64_t tail __attribute__((unused));

    for (i = 0; i < MPMC_DEGREE && q != 0; ++i) {
        head = dma_read(q, offsetof(sm_queue_t, head), uint64_t) - 1;
        tail = dma_read(q, offsetof(sm_queue_t, tail), uint64_t);
        head_slot = head % SM_QUEUE_SIZE;
        dma_write(q, offsetof(sm_queue_t, slot[head_slot]) + offsetof(sm_queue_slot_t, entry), entry, uint64_t);
        dma_write(q, offsetof(sm_queue_t, slot[head_slot]) + offsetof(sm_queue_slot_t, seq), head + 1, uint64_t);
        q = dma_read(q, offsetof(sm_queue_t, next_queue_ptr), uint64_t);
    }
}

int sm_queue_can_pop(uint64_t q, void *data, uint64_t size) {
    int i;
    uint64_t tail, tail_slot, seq, entry, next_q;

    if (q == 0) {
        return false;
    }

    for (i = 0; i < MPMC_DEGREE; ++i) {
        tail = dma_read(q, offsetof(sm_queue_t, tail), uint64_t);
        tail_slot = tail % SM_QUEUE_SIZE;
        seq = dma_read(q, offsetof(sm_queue_t, slot[tail_slot]) + offsetof(sm_queue_slot_t, seq), uint64_t);
        if (seq != (tail + 1)) {
            return false;
        }

        next_q = dma_read(q, offsetof(sm_queue_t, next_queue_ptr), uint64_t);
        if (next_q == 0) {
            break;
        }
        q = next_q;
    }

    entry = dma_read(q, offsetof(sm_queue_t, slot[tail_slot]) + offsetof(sm_queue_slot_t, entry), uint64_t) ;
    dma_memory_read(&address_space_memory, 0xa0200000 + entry * sizeof(uint32_t), data, size, MEMTXATTRS_UNSPECIFIED); /* TODO: make base address elegant */

    return true;
}

void sm_queue_pop(uint64_t q) {
    int i;
    uint64_t tail;

    for (i = 0; i < MPMC_DEGREE && q != 0; ++i) {
        tail = dma_read(q, offsetof(sm_queue_t, tail), uint64_t);
        dma_write(q, offsetof(sm_queue_t, slot[tail % SM_QUEUE_SIZE]) + offsetof(sm_queue_slot_t, seq), tail + SM_QUEUE_SIZE, uint64_t);
        dma_write(q, offsetof(sm_queue_t, tail), tail + 1, uint64_t);
        q = dma_read(q, offsetof(sm_queue_t, next_queue_ptr), uint64_t);
    }
}
