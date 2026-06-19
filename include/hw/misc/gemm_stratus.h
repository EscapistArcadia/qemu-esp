#ifndef HW_GEMM_STRATUS_H
#define HW_GEMM_STRATUS_H

#include "qemu/osdep.h"
#include "qom/object.h"
#include "hw/core/sysbus.h"
#include "hw/core/qdev.h"

#define TYPE_GEMM_STRATUS "gemm_stratus"

struct GemmStratusState {
    SysBusDevice parent_obj;

    MemoryRegion mmio;
    qemu_irq irq;
};

typedef struct GemmStratusState GemmStratusState;
DECLARE_INSTANCE_CHECKER(GemmStratusState, GEMM_STRATUS, TYPE_GEMM_STRATUS)

#endif
