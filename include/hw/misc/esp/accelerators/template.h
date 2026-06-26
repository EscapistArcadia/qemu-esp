#ifndef ESP_ACCELERATOR_TEMPLATE_H
#define ESP_ACCELERATOR_TEMPLATE_H

#include <stdint.h>

typedef struct ESPAcceleratorState ESPAcceleratorState; /* Learning: this can fix recursive includes */

typedef struct ESPAccelerator {
    uint64_t conf_size;

    void (*execute)(ESPAcceleratorState *s, void *param);
} ESPAccelerator;

#endif
