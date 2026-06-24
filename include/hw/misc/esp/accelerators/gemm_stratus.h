#ifndef ESP_GEMM_STRATUS_H
#define ESP_GEMM_STRATUS_H

#include "hw/misc/esp/accelerator.h"

#define TYPE_GEMM_STRATUS "gemm_stratus"

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

struct GemmStratusState {
    ESPAcceleratorState parent_obj;

    uint32_t ninputs;
    uint32_t d1, d2, d3;
    uint32_t ld_offset1, ld_offset2;
    uint32_t bias_offset;
    uint32_t st_offset;
    uint32_t do_relu;
    uint32_t do_bias;
    uint32_t transpose;
};

typedef struct GemmStratusState GemmStratusState;
DECLARE_INSTANCE_CHECKER(GemmStratusState, GEMM_STRATUS, TYPE_GEMM_STRATUS)

DeviceState *gemm_stratus_create(void);

#endif
