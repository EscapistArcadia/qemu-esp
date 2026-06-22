#ifndef HW_GEMM_STRATUS_H
#define HW_GEMM_STRATUS_H

#include "qemu/osdep.h"
#include "qom/object.h"
#include "hw/core/sysbus.h"
#include "hw/core/qdev.h"

#define TYPE_GEMM_STRATUS "gemm_stratus"

#define GEMM_MMIO_BASE (0x60010000)
#define GEMM_MMIO_SIZE (0x100)
#define GEMM_IRQ (6)

#define CONTEXT_COUNT 4

struct GemmStratusState {
    SysBusDevice parent_obj;

    MemoryRegion mmio;
    qemu_irq irq;

    uint32_t pt_address_high;
    uint32_t pt_address_low[CONTEXT_COUNT];
    uint64_t queue_ptr[CONTEXT_COUNT];
    uint32_t nprio[CONTEXT_COUNT];
    uint32_t valid_contexts;
    uint32_t sched_period;

    QemuThread gemm_eq;
};

typedef struct GemmStratusState GemmStratusState;
DECLARE_INSTANCE_CHECKER(GemmStratusState, GEMM_STRATUS, TYPE_GEMM_STRATUS)

DeviceState *gemm_stratus_create(void);

#endif
