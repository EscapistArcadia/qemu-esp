#include <stddef.h>
#include <string.h>
#include "hw/misc/esp/accelerators/template.h"

static QLIST_HEAD(ESPAcceleratorList, ESPAccelerator) accelerators = QLIST_HEAD_INITIALIZER(); /* TODO: Use of undeclared identifier 'NULL' if no stddef.h, adding this header makes code messy */

void esp_accelerator_type_register(ESPAccelerator *accel) {
    QLIST_INSERT_HEAD(&accelerators, accel, node);
}

ESPAccelerator *esp_accelerator_type(const char *type) {
    ESPAccelerator *accel;
    QLIST_FOREACH(accel, &accelerators, node) {
        if (strcmp(accel->type, type) == 0) { /* TODO: here, strcmp */
            return accel;
        }
    }
    return NULL;
}
