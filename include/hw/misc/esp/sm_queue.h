#ifndef VIRT_SM_QUEUE_H
#define VIRT_SM_QUEUE_H

#include <stdint.h>

#define SM_QUEUE_SIZE 4

typedef struct {
    unsigned output_queue;
    unsigned output_entry;
} sm_queue_entry_t;

typedef struct {
    uint64_t seq;
    uint64_t entry;
} sm_queue_slot_t;

typedef struct {
    uint64_t stat;
    uint64_t head;
    uint64_t tail;
    uint64_t next_queue_ptr;
    sm_queue_slot_t slot[SM_QUEUE_SIZE];
} sm_queue_t;

int sm_queue_can_push(uint64_t q, uint64_t head_idx);

void sm_queue_push(uint64_t q, uint64_t head_idx, uint64_t value);

int sm_queue_can_pop(uint64_t q, uint64_t *value);

uint64_t sm_queue_pop(uint64_t q);

#endif
