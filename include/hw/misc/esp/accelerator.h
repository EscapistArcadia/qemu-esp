#ifndef ESP_ACCELERATOR_H
#define ESP_ACCELERATOR_H

#include "hw/misc/esp/subsystem.h"
#include "hw/misc/esp/accelerators/template.h"

#define TYPE_ESP_ACCELERATOR "esp_accelerator"

#define MAX_CONTEXTS 4
#define MPMC_DEGREE 2

struct ESPAcceleratorState {
    // ESPSubsystemState parent_obj;
    SysBusDevice parent_obj;

    /* Subsystem object to access shared memory */
    ESPSubsystemState *esp;

    /* Accelerator information */
    ESPAccelerator *accel;

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

    /* Scheduler state (mirrors esp_accelerator_amu member variables) */
    uint32_t current_context;
    uint32_t current_context_next;          /* RR: starting index for next round */
    uint32_t context_vruntime[MAX_CONTEXTS];
    uint64_t context_runtime[MAX_CONTEXTS];
    uint32_t valid_queue_ptr;               /* bitmap: contexts with BUSY queues */
    uint32_t prev_valid_contexts;           /* previous valid_contexts for transition detection */

    QLIST_ENTRY(ESPAcceleratorState) node;
};

typedef struct ESPAcceleratorState ESPAcceleratorState;
DECLARE_INSTANCE_CHECKER(ESPAcceleratorState, ESP_ACCELERATOR, TYPE_ESP_ACCELERATOR)

DeviceState *esp_accelerator_create(ESPSubsystemState *esp, ESPAccelerator *accel, hwaddr mmio_base, uint64_t mmio_size);

void pick_context_rr(ESPAcceleratorState *s, bool bias_switch);
void pick_context_fair(ESPAcceleratorState *s, bool bias_switch);

#endif
