#include "hw/misc/esp/accelerators/template.h"
#include "hw/misc/esp/regmap.h"
#include "hw/misc/esp/sm_queue.h"
#include "hw/misc/esp/helper/dma.h"

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
    sm_queue_entry_t output[MPMC_DEGREE];
} GemmStratusEntry;

static uint64_t gemm_stratus_mmio_read(ESPAcceleratorState *s, hwaddr addr, unsigned size) {
    return 0;
}

static void gemm_stratus_mmio_write(ESPAcceleratorState *s, hwaddr addr, uint64_t data, unsigned size) {

}

static void gemm_stratus_execute(ESPAcceleratorState *s, void *param) {
    GemmStratusConf *p = (GemmStratusConf *)param;

    uint32_t n = p->ninputs;
    uint32_t d1 = p->d1, d2 = p->d2, d3 = p->d3;

    uint64_t base = s->esp->sm.addr;
    size_t size_a = (size_t)n * d1 * d2 * sizeof(int32_t);
    size_t size_b = (size_t)n * d2 * d3 * sizeof(int32_t);
    size_t size_c = (size_t)n * d1 * d3 * sizeof(int32_t);

    int32_t *a    = g_malloc(size_a);
    int32_t *b    = g_malloc(size_b);
    int32_t *c    = g_malloc0(size_c);
    int32_t *bias = p->do_bias ? g_malloc(d3 * sizeof(int32_t)) : NULL;

    dma_memory_read(&address_space_memory,
                    base + (uint64_t)p->ld_offset1 * sizeof(int32_t), a, size_a,
                    MEMTXATTRS_UNSPECIFIED);
    dma_memory_read(&address_space_memory,
                    base + (uint64_t)p->ld_offset2 * sizeof(int32_t), b, size_b,
                    MEMTXATTRS_UNSPECIFIED);
    if (p->do_bias) {
        dma_memory_read(&address_space_memory,
                        base + (uint64_t)p->bias_offset * sizeof(int32_t), bias,
                        d3 * sizeof(int32_t), MEMTXATTRS_UNSPECIFIED);
    }

    for (uint32_t batch = 0; batch < n; batch++) {
        int32_t *A = a + (size_t)batch * d1 * d2;
        int32_t *B = b + (size_t)batch * d2 * d3;
        int32_t *C = c + (size_t)batch * d1 * d3;

        for (uint32_t i = 0; i < d1; i++) {
            for (uint32_t j = 0; j < d3; j++) {
                int64_t acc = 0;
                for (uint32_t k = 0; k < d2; k++) {
                    /* B stored as d3×d2 when transposed, d2×d3 otherwise */
                    int32_t b_val = p->transpose ? B[j * d2 + k] : B[k * d3 + j];
                    acc += ((int64_t)A[i * d2 + k] * b_val) >> 24;
                }
                if (p->do_bias) {
                    acc += bias[j];
                }
                if (p->do_relu && acc < 0) {
                    acc = 0;
                }
                C[i * d3 + j] = (int32_t)acc;
            }
        }
    }

    dma_memory_write(&address_space_memory,
                     base + (uint64_t)p->st_offset * sizeof(int32_t), c, size_c,
                     MEMTXATTRS_UNSPECIFIED);

    g_free(a);
    g_free(b);
    g_free(c);
    g_free(bias);
}

ESPAccelerator gemm_stratus_rr = {
    .conf_size = sizeof(GemmStratusConf),

    .mmio_read = gemm_stratus_mmio_read,
    .mmio_write = gemm_stratus_mmio_write,

    .pick_context = pick_context_rr,
    .execute = gemm_stratus_execute,
};

ESPAccelerator gemm_stratus_fair = {
    .conf_size = sizeof(GemmStratusConf),

    .mmio_read = gemm_stratus_mmio_read,
    .mmio_write = gemm_stratus_mmio_write,

    .pick_context = pick_context_fair,
    .execute = gemm_stratus_execute,
};
