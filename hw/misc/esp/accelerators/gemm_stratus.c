// #include "hw/misc/esp/accelerators/gemm_stratus.h"
#include "hw/misc/esp/regmap.h"

// static const TypeInfo gemm_stratus_info = {
//     .name = TYPE_GEMM_STRATUS,
//     .parent = TYPE_ESP_ACCELERATOR,
//     .instance_size = sizeof(GemmStratusState),
// };

// static void gemm_stratus_register_types(void) {
//     type_register_static(&gemm_stratus_info);
// }

// type_init(gemm_stratus_register_types)



// #define TYPE_GEMM_STRATUS "gemm_stratus"

enum GemmStratusMMIORegisters {
    GEMM_NINPUTS_REG = ESP_ACCELERATOR_MMIO_MAX + 0x04,
    GEMM_D1_REG = ESP_ACCELERATOR_MMIO_MAX + 0x08,
    GEMM_D2_REG = ESP_ACCELERATOR_MMIO_MAX + 0x0c,
    GEMM_D3_REG = ESP_ACCELERATOR_MMIO_MAX + 0x10,
    GEMM_LD_OFFSET1_REG = ESP_ACCELERATOR_MMIO_MAX + 0x14,
    GEMM_LD_OFFSET2_REG = ESP_ACCELERATOR_MMIO_MAX + 0x18,
    GEMM_BIAS_OFFSET_REG = ESP_ACCELERATOR_MMIO_MAX + 0x1c,
    GEMM_ST_OFFSET_REG = ESP_ACCELERATOR_MMIO_MAX + 0x20,
    GEMM_DO_RELU_REG = ESP_ACCELERATOR_MMIO_MAX + 0x24,
    GEMM_DO_BIAS_REG = ESP_ACCELERATOR_MMIO_MAX + 0x28,
    GEMM_TRANSPOSE_REG = ESP_ACCELERATOR_MMIO_MAX + 0x2c,
};