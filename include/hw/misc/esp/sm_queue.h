#ifndef VIRT_SM_QUEUE_H
#define VIRT_SM_QUEUE_H

#include <stdint.h>
#include "hw/misc/esp/accelerator.h"

#define SM_QUEUE_SIZE 4

#define QUEUE_INVALID 0
#define QUEUE_AVAIL 1
#define QUEUE_BUSY 2

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

int sm_queue_can_push(ESPAcceleratorState *s, uint64_t q);

void sm_queue_push(ESPAcceleratorState *s, uint64_t q, uint64_t entry);

int sm_queue_can_pop(ESPAcceleratorState *s, uint64_t q, void *data, uint64_t size);

void sm_queue_pop(ESPAcceleratorState *s, uint64_t q);

#endif
