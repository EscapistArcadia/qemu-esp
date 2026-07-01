#ifndef ESP_SUBSYSTEM_H
#define ESP_SUBSYSTEM_H

#include "qemu/osdep.h"
#include "qom/object.h"
#include "hw/core/sysbus.h"
#include "hw/core/qdev.h"
#include "qemu/queue.h"

#define TYPE_ESP_SUBSYSTEM "esp_subsystem"

struct ESPSubsystemState {
    SysBusDevice parent_obj;

    /* GRETH memory region, unused */
    MemoryRegion greth;

    /* Shared memory region */
    MemoryRegion sm;

    /* Accelerator instance list */
    QLIST_HEAD(, ESPAcceleratorState) accelerators;
};

typedef struct ESPSubsystemState ESPSubsystemState;
DECLARE_INSTANCE_CHECKER(ESPSubsystemState, ESP_SUBSYSTEM, TYPE_ESP_SUBSYSTEM)

/**
 * @brief Initializes the ESP accelerator subsystem and returns a pointer of the base subsystem object.
 * 
 * @return DeviceState* the base subsystem object pointer
 * @todo automate the address realization and memory region mapping based on the passed dtb or fdt
 */
DeviceState *esp_subsystem_init(void *fdt);

#endif
