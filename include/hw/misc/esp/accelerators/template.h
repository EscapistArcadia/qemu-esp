#ifndef ESP_ACCELERATOR_TEMPLATE_H
#define ESP_ACCELERATOR_TEMPLATE_H

#include <stdint.h>
#include <stdbool.h>
#include "exec/hwaddr.h"
#include "qemu/queue.h"

typedef struct ESPAcceleratorState ESPAcceleratorState; /* Learning: this can fix recursive includes */
typedef uint64_t (*ESPAcceleratorMMIORead)(ESPAcceleratorState *s, hwaddr addr, unsigned size);
typedef void (*ESPAcceleratorMMIOWrite)(ESPAcceleratorState *s, hwaddr addr, uint64_t data, unsigned size);
typedef void (*ESPAcceleratorPickContext)(ESPAcceleratorState *s, bool bias_switch);
typedef void (*ESPAcceleratorExecute)(ESPAcceleratorState *s, void *param);

/* TODO: add baseline support, but not necessary for now */
typedef struct ESPAccelerator {
    const char *type; /* name of the accelerator shown in dtb */

    uint64_t conf_size;

    ESPAcceleratorMMIORead mmio_read;
    ESPAcceleratorMMIOWrite mmio_write;

    ESPAcceleratorPickContext pick_context;
    ESPAcceleratorExecute execute;

    QLIST_ENTRY(ESPAccelerator) node; /* for the global list of registered accelerators */
} ESPAccelerator;

void esp_accelerator_type_register(ESPAccelerator *accel);
ESPAccelerator *esp_accelerator_type(const char *type);

#endif
