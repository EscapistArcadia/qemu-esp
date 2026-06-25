#ifndef ESP_ACCELERATOR_H
#define ESP_ACCELERATOR_H

#include "hw/misc/esp/subsystem.h"

#define TYPE_ESP_ACCELERATOR "esp_accelerator"

#define MAX_CONTEXTS 4
#define MPMC_DEGREE 2

struct ESPAcceleratorState {
    // ESPSubsystemState parent_obj;
    SysBusDevice parent_obj;

    /* Subsystem object to access shared memory */
    ESPSubsystemState *esp;

    /* Accelerator MMIO region */
    MemoryRegion mmio;

    QemuThread execution; /* Accelerator execution flow emulation */
    QemuThread valid_context_monitor; /* TODO: make sure the implementation is equivalent to hardware (o_O)? */
    QemuThread cycle_counter; /* TODO: do we really need this?*/

    /* Accelerator-independent MMIO registers */
    uint32_t pt_address_high;
    uint32_t pt_address_low[MAX_CONTEXTS];
    uint32_t queue_ptr[MAX_CONTEXTS];
    uint32_t nprio[MAX_CONTEXTS];
    uint32_t valid_contexts;
    uint32_t sched_period;

    /* Execution flow state information */
    uint64_t curr_tail_idx[MPMC_DEGREE];
    uint64_t next_head_idx[MPMC_DEGREE];
    uint64_t sm_info_output_queue[MPMC_DEGREE];
    uint64_t sm_info_output_entry[MPMC_DEGREE];
};

typedef struct ESPAcceleratorState ESPAcceleratorState;
DECLARE_INSTANCE_CHECKER(ESPAcceleratorState, ESP_ACCELERATOR, TYPE_ESP_ACCELERATOR)

DeviceState *esp_accelerator_create(ESPSubsystemState *esp, const char *type, hwaddr mmio_base, uint64_t mmio_size);

#endif
