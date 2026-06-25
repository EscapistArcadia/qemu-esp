#ifndef ESP_GEMM_STRATUS_H
#define ESP_GEMM_STRATUS_H

#include "hw/misc/esp/accelerator.h"
#include "hw/misc/esp/sm_queue.h"

typedef struct GemmStratusConf {
    uint32_t ninputs;
    uint32_t d1, d2, d3;
    uint32_t ld_offset1, ld_offset2;
    uint32_t bias_offset;
    uint32_t st_offset;
    uint32_t do_relu;
    uint32_t do_bias;
    uint32_t transpose;
    uint32_t padding;
} GemmStratusConf;

typedef struct GemmStratusEntry {
    GemmStratusConf gemm_params;
    sm_queue_entry_t output[2];
} GemmStratusEntry;

// struct GemmStratusState {
//     ESPAcceleratorState parent_obj;

//     GemmStratusConf gemm_params;
// };

// typedef struct GemmStratusState GemmStratusState;
// DECLARE_INSTANCE_CHECKER(GemmStratusState, GEMM_STRATUS, TYPE_GEMM_STRATUS)

// DeviceState *gemm_stratus_create(void);

#endif
