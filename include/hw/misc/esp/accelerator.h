#ifndef ESP_ACCELERATOR_H
#define ESP_ACCELERATOR_H

#include "hw/misc/esp/subsystem.h"
#include "hw/misc/esp/sm_queue.h"

#define TYPE_ESP_ACCELERATOR "esp_accelerator"

#define MAX_CONTEXTS 4

enum ESPAcceleratorMMIORegisters {
    CMD_REG = 0x00,
    STATUS_REG = 0x04,
    SELECT_REG = 0x08,
    DEVID_REG = 0x0c,
    PT_ADDRESS_REG = 0x10,
    PT_NCHUNK_REG = 0x14,
    PT_SHIFT_REG = 0x18,
    PT_NCHUNK_MAX_REG = 0x1c,
    PT_ADDRESS_EXTENDED_REG = 0x20,
    PT_COHERENCE_REG = 0x24,
    P2P_REG = 0x28,
    YX_REG = 0x2c,
    SRC_OFFSET_REG = 0x30,
    DST_OFFSET_REG = 0x34,
    ESP_MAX = DST_OFFSET_REG,

    /* following: vignesh's fantastic additions */
    SPANDEX_REG = 0x38,

    VALID_CONTEXTS_ACK_REG = 0x3c,
    
    PT_ADDRESS_REG_0 = 0x40,
    PT_ADDRESS_REG_1 = 0x44,
    PT_ADDRESS_REG_2 = 0x48,
    PT_ADDRESS_REG_3 = 0x4c,

    MON_UTIL_REG_0_LO = 0x50,
    MON_UTIL_REG_0_HI = 0x54,
    MON_UTIL_REG_1_LO = 0x58,
    MON_UTIL_REG_1_HI = 0x5c,
    MON_UTIL_REG_2_LO = 0x60,
    MON_UTIL_REG_2_HI = 0x64,
    MON_UTIL_REG_3_LO = 0x68,
    MON_UTIL_REG_3_HI = 0x6c,

    AMU_INFO_QUEUE_PTR_REG_0 = 0x70,
    AMU_INFO_QUEUE_PTR_REG_1 = 0x74,
    AMU_INFO_QUEUE_PTR_REG_2 = 0x78,
    AMU_INFO_QUEUE_PTR_REG_3 = 0x7c,

    AMU_INFO_NPRIO_REG_0 = 0x80,
    AMU_INFO_NPRIO_REG_1 = 0x84,
    AMU_INFO_NPRIO_REG_2 = 0x88,
    AMU_INFO_NPRIO_REG_3 = 0x8c,

    AMU_INFO_VLD_CTXT_REG = 0x90,
    AMU_INFO_SCHED_PERIOD_REG = 0x94,

    ESP_ACCELERATOR_MMIO_MAX = AMU_INFO_SCHED_PERIOD_REG,
};

struct ESPAcceleratorState {
    ESPSubsystemState parent_obj;

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
};

typedef struct ESPAcceleratorState ESPAcceleratorState;
DECLARE_INSTANCE_CHECKER(ESPAcceleratorState, ESP_ACCELERATOR, TYPE_ESP_ACCELERATOR)

DeviceState *esp_accelerator_create(const char *type, hwaddr mmio_base, uint64_t mmio_size);

#endif
