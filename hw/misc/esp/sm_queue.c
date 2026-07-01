#include "hw/misc/esp/sm_queue.h"

#include "hw/misc/esp/helper/dma.h"
#include "system/memory.h"

#define MPMC_DEGREE 2
#define SM_ENTRY_SIZE (2 * MPMC_DEGREE)
#define SM_SEQ_SIZE 2
#define SM_COMMON_SIZE 8
#define SM_SLOT_SIZE 4
#define SM_QUEUE_WORDS (SM_COMMON_SIZE + (SM_SLOT_SIZE * SM_QUEUE_SIZE))

/** 
 * TODO: I just saw memory_region_get_ram_ptr, which returns the base pointer of the RAM region 
 * However, from the comments, it seems that this function is dangerous to use, as calling it stops RCU protection. I don't know what RCU is, but after have a deeper understanding, I should come back and check if it can make the emulation faster.
 */

int sm_queue_can_push(ESPAcceleratorState *s, uint64_t q) {
    int i;
    uint64_t head, tail, head_slot, seq, expected_head;
    uint64_t current_queue;
    bool retry;

    if (q == 0) {
        return false;
    }

    for (i = 0; i < MPMC_DEGREE; ++i) {
        current_queue = s->sm_info_output_queue[i];
        if (current_queue == 0) {
            break;
        }

        head = dma_read(current_queue, offsetof(sm_queue_t, head), uint64_t);
        tail = dma_read(current_queue, offsetof(sm_queue_t, tail), uint64_t);
        if ((head - tail) >= SM_QUEUE_SIZE) {
            return false;
        }

        head_slot = head % SM_QUEUE_SIZE;
        seq = dma_read(current_queue, offsetof(sm_queue_t, slot[head_slot]) + offsetof(sm_queue_slot_t, seq), uint64_t);
        if (seq != head) {
            return false;
        }

        s->next_head_idx[i] = head;
    }

    for (i = 0; i < MPMC_DEGREE; ++i) {
        current_queue = s->sm_info_output_queue[i];
        if (current_queue == 0) {
            break;
        }

        retry = false;
        while (true) {
            head = s->next_head_idx[i];
            if (retry) {
                head_slot = head % SM_QUEUE_SIZE;
                seq = dma_read(current_queue, offsetof(sm_queue_t, slot[head_slot]) + offsetof(sm_queue_slot_t, seq), uint64_t);
                if (seq != head) {
                    continue;
                }
            }

            expected_head = dma_read(current_queue, offsetof(sm_queue_t, head), uint64_t);
            if (head != expected_head) {
                s->next_head_idx[i] = expected_head;
                retry = true;
                continue;
            }

            dma_write(current_queue, offsetof(sm_queue_t, head), head + 1, uint64_t);
            break;
        }
    }

    return true;
}

void sm_queue_push(ESPAcceleratorState *s, uint64_t q, uint64_t entry) {
    int i;
    uint64_t head, head_slot;
    uint64_t current_queue;

    for (i = 0; i < MPMC_DEGREE; ++i) {
        current_queue = s->sm_info_output_queue[i];
        if (current_queue == 0) {
            break;
        }
        head = s->next_head_idx[i];
        head_slot = head % SM_QUEUE_SIZE;
        dma_write(current_queue, offsetof(sm_queue_t, slot[head_slot]) + offsetof(sm_queue_slot_t, entry), s->sm_info_output_entry[i], uint64_t);
        dma_write(current_queue, offsetof(sm_queue_t, slot[head_slot]) + offsetof(sm_queue_slot_t, seq), head + 1, uint64_t);
    }
}

int sm_queue_can_pop(ESPAcceleratorState *s, uint64_t q, void *data, uint64_t size) {
    int i;
    uint64_t tail, tail_slot, seq, entry, base;
    uint64_t current_queue = q, next_queue;
    // volatile void *ram_ptr __attribute__((unused));

    if (q == 0) {
        /* TODO: check the necessity of valid_queue_ptr[MAX_CONTEXTS] */
        return false;
    }

    for (i = 0; i < MPMC_DEGREE; ++i) {
        s->curr_tail_idx[i] = tail = dma_read(current_queue, offsetof(sm_queue_t, tail), uint64_t);
        tail_slot = tail % SM_QUEUE_SIZE;
        next_queue = dma_read(current_queue, offsetof(sm_queue_t, next_queue_ptr), uint64_t);
        if (next_queue != 0)
            next_queue = s->esp->sm.addr + next_queue * sizeof(uint32_t); /* TODO: find a more elegant way to handle this */
        seq = dma_read(current_queue, offsetof(sm_queue_t, slot[tail_slot]) + offsetof(sm_queue_slot_t, seq), uint64_t);
        if (seq != (tail + 1)) {
            return false;
        }

        if (next_queue == 0) {
            break;
        }
        current_queue = next_queue;
    }

    tail_slot = tail % SM_QUEUE_SIZE;
    base = s->esp->sm.addr;
    /* TODO: ram_ptr makes sense, I will refactor the access pattern later. */
    // ram_ptr = memory_region_get_ram_ptr(&s->esp->sm);
    entry = dma_read(current_queue, offsetof(sm_queue_t, slot[tail_slot]) + offsetof(sm_queue_slot_t, entry), uint64_t) * sizeof(uint32_t);
    dma_memory_read(&address_space_memory, base + entry, data, size, MEMTXATTRS_UNSPECIFIED);

    sm_queue_entry_t *output_info = (sm_queue_entry_t *)((uint8_t *)data + size) - MPMC_DEGREE;
    for (i = 0; i < MPMC_DEGREE; ++i) {
        s->sm_info_output_queue[i] = output_info[i].output_queue ? base + output_info[i].output_queue * sizeof(uint32_t) : 0;
        s->sm_info_output_entry[i] = output_info[i].output_entry;
    }
    return true;
}

void sm_queue_pop(ESPAcceleratorState *s, uint64_t q) {
    int i;
    uint64_t tail, tail_slot;
    uint64_t current_queue = q;

    for (i = 0; i < MPMC_DEGREE && current_queue != 0; ++i) {
        tail = s->curr_tail_idx[i];
        tail_slot = tail % SM_QUEUE_SIZE;
        dma_write(current_queue, offsetof(sm_queue_t, slot[tail_slot]) + offsetof(sm_queue_slot_t, seq), tail + SM_QUEUE_SIZE, uint64_t);
        dma_write(current_queue, offsetof(sm_queue_t, tail), tail + 1, uint64_t);
        current_queue = dma_read(current_queue, offsetof(sm_queue_t, next_queue_ptr), uint64_t);
        if (current_queue != 0)
            current_queue = s->esp->sm.addr + current_queue * sizeof(uint32_t); /* TODO: find a more elegant way to handle this */
    }
}
