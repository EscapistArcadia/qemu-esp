#include "hw/misc/esp/accelerators/gemm_stratus.h"

static const TypeInfo gemm_stratus_info = {
    .name = TYPE_GEMM_STRATUS,
    .parent = TYPE_ESP_ACCELERATOR,
    .instance_size = sizeof(GemmStratusState),
};

static void gemm_stratus_register_types(void) {
    type_register_static(&gemm_stratus_info);
}

type_init(gemm_stratus_register_types)
