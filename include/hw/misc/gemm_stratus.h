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

struct GemmStratusState {
    SysBusDevice parent_obj;

    MemoryRegion mmio;
    qemu_irq irq;
};

typedef struct GemmStratusState GemmStratusState;
DECLARE_INSTANCE_CHECKER(GemmStratusState, GEMM_STRATUS, TYPE_GEMM_STRATUS)

DeviceState *gemm_stratus_create(void);

#endif
