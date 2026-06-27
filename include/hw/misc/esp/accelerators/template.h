#ifndef ESP_ACCELERATOR_TEMPLATE_H
#define ESP_ACCELERATOR_TEMPLATE_H

#include <stdint.h>
#include <stdbool.h>
#include "exec/hwaddr.h"

typedef struct ESPAcceleratorState ESPAcceleratorState; /* Learning: this can fix recursive includes */
typedef uint64_t (*ESPAcceleratorMMIORead)(ESPAcceleratorState *s, hwaddr addr, unsigned size);
typedef void (*ESPAcceleratorMMIOWrite)(ESPAcceleratorState *s, hwaddr addr, uint64_t data, unsigned size);
typedef void (*ESPAcceleratorPickContext)(ESPAcceleratorState *s, bool bias_switch);
typedef void (*ESPAcceleratorExecute)(ESPAcceleratorState *s, void *param);

typedef struct ESPAccelerator {
    uint64_t conf_size;

    ESPAcceleratorMMIORead mmio_read;
    ESPAcceleratorMMIOWrite mmio_write;

    ESPAcceleratorPickContext pick_context;
    ESPAcceleratorExecute execute;
} ESPAccelerator;

#endif
