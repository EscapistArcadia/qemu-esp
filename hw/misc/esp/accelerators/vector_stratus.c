#include "hw/misc/esp/accelerators/template.h"
#include "hw/misc/esp/regmap.h"
#include "hw/misc/esp/sm_queue.h"
#include "hw/misc/esp/helper/dma.h"

#define VECTOR_OP_ADD      0
#define VECTOR_OP_AVG_POOL 1

enum VectorStratusMMIORegisters {
    VECTOR_OP_REG           = ESP_ACCELERATOR_MMIO_MAX + 0x04,
    VECTOR_N_CHANNEL_REG    = ESP_ACCELERATOR_MMIO_MAX + 0x08,
    VECTOR_INPUT_HEIGHT_REG = ESP_ACCELERATOR_MMIO_MAX + 0x0c,
    VECTOR_INPUT_WIDTH_REG  = ESP_ACCELERATOR_MMIO_MAX + 0x10,
    VECTOR_KERNEL_HEIGHT_REG= ESP_ACCELERATOR_MMIO_MAX + 0x14,
    VECTOR_KERNEL_WIDTH_REG = ESP_ACCELERATOR_MMIO_MAX + 0x18,
    VECTOR_INPUT1_OFFSET_REG= ESP_ACCELERATOR_MMIO_MAX + 0x1c,
    VECTOR_INPUT2_OFFSET_REG= ESP_ACCELERATOR_MMIO_MAX + 0x20,
    VECTOR_OUTPUT_OFFSET_REG= ESP_ACCELERATOR_MMIO_MAX + 0x24,
    VECTOR_DO_RELU_REG      = ESP_ACCELERATOR_MMIO_MAX + 0x28,
};

typedef struct VectorStratusConf {
    uint32_t vector_op;
    uint32_t n_channel;
    uint32_t input_height;  /* vector length for add; feature-map height for avg pool */
    uint32_t input_width;   /* unused for add */
    uint32_t kernel_height; /* unused for add */
    uint32_t kernel_width;  /* unused for add */
    uint32_t input1_offset;
    uint32_t input2_offset; /* unused for avg pool */
    uint32_t output_offset;
    uint32_t do_relu;
} VectorStratusConf;

typedef struct VectorStratusEntry {
    VectorStratusConf vector_params;
    sm_queue_entry_t output[MPMC_DEGREE];
} VectorStratusEntry;

static uint64_t vector_stratus_mmio_read(ESPAcceleratorState *s, hwaddr addr, unsigned size) {
    return 0;
}

static void vector_stratus_mmio_write(ESPAcceleratorState *s, hwaddr addr, uint64_t data, unsigned size) {
}

static void vector_stratus_execute(ESPAcceleratorState *s, void *param) {
    VectorStratusConf *p = (VectorStratusConf *)param;
    uint64_t base = s->esp->sm.addr;

    /*
     * Fixed-point format: Q8.24 (32-bit signed, 8 integer bits, 24 fractional bits).
     * Element-wise add is exact in integer arithmetic (same radix point).
     * Average pool divides the integer sum by kernel_area.
     */
    switch (p->vector_op) {
    case VECTOR_OP_ADD: {
        uint32_t len = p->input_height;
        int32_t *in1 = g_malloc(len * sizeof(int32_t));
        int32_t *in2 = g_malloc(len * sizeof(int32_t));
        int32_t *out = g_malloc(len * sizeof(int32_t));

        dma_memory_read(&address_space_memory,
                        base + (uint64_t)p->input1_offset * sizeof(int32_t),
                        in1, len * sizeof(int32_t), MEMTXATTRS_UNSPECIFIED);
        dma_memory_read(&address_space_memory,
                        base + (uint64_t)p->input2_offset * sizeof(int32_t),
                        in2, len * sizeof(int32_t), MEMTXATTRS_UNSPECIFIED);

        for (uint32_t i = 0; i < len; i++) {
            int32_t res = in1[i] + in2[i];
            if (p->do_relu && res < 0) {
                res = 0;
            }
            out[i] = res;
        }

        dma_memory_write(&address_space_memory,
                         base + (uint64_t)p->output_offset * sizeof(int32_t),
                         out, len * sizeof(int32_t), MEMTXATTRS_UNSPECIFIED);

        g_free(in1);
        g_free(in2);
        g_free(out);
        break;
    }
    case VECTOR_OP_AVG_POOL: {
        /*
         * Input layout : [n_channel][input_height][input_width]
         * Output layout: [n_channel][out_h][out_w]
         * where out_h = input_height / kernel_height,
         *       out_w = input_width  / kernel_width.
         */
        uint32_t n_ch   = p->n_channel;
        uint32_t in_h   = p->input_height;
        uint32_t in_w   = p->input_width;
        uint32_t kh     = p->kernel_height;
        uint32_t kw     = p->kernel_width;
        uint32_t out_h  = in_h / kh;
        uint32_t out_w  = in_w / kw;
        uint32_t karea  = kh * kw;

        size_t sz_in  = (size_t)n_ch * in_h  * in_w;
        size_t sz_out = (size_t)n_ch * out_h * out_w;

        int32_t *input  = g_malloc(sz_in  * sizeof(int32_t));
        int32_t *output = g_malloc(sz_out * sizeof(int32_t));

        dma_memory_read(&address_space_memory,
                        base + (uint64_t)p->input1_offset * sizeof(int32_t),
                        input, sz_in * sizeof(int32_t), MEMTXATTRS_UNSPECIFIED);

        for (uint32_t c = 0; c < n_ch; c++) {
            for (uint32_t r = 0; r < out_h; r++) {
                for (uint32_t col = 0; col < out_w; col++) {
                    int64_t sum = 0;
                    for (uint32_t pr = 0; pr < kh; pr++) {
                        for (uint32_t pc = 0; pc < kw; pc++) {
                            uint32_t idx = c * in_h * in_w +
                                           (r * kh + pr) * in_w +
                                           (col * kw + pc);
                            sum += (int64_t)input[idx];
                        }
                    }
                    output[c * out_h * out_w + r * out_w + col] =
                        (int32_t)(sum / (int64_t)karea);
                }
            }
        }

        dma_memory_write(&address_space_memory,
                         base + (uint64_t)p->output_offset * sizeof(int32_t),
                         output, sz_out * sizeof(int32_t), MEMTXATTRS_UNSPECIFIED);

        g_free(input);
        g_free(output);
        break;
    }
    default:
        break;
    }
}

ESPAccelerator vector_stratus_rr = {
    .type = "vector_stratus",

    .conf_size = sizeof(VectorStratusConf),

    .mmio_read  = vector_stratus_mmio_read,
    .mmio_write = vector_stratus_mmio_write,

    .pick_context = pick_context_rr,
    .execute      = vector_stratus_execute,
};

ESPAccelerator vector_stratus_fair = {
    .type = "vector_stratus",

    .conf_size = sizeof(VectorStratusConf),

    .mmio_read  = vector_stratus_mmio_read,
    .mmio_write = vector_stratus_mmio_write,

    .pick_context = pick_context_fair,
    .execute      = vector_stratus_execute,
};

static void vector_stratus_register(void) {
    /* TODO: check configuration, rr by default */
    esp_accelerator_type_register(&vector_stratus_rr);
}

type_init(vector_stratus_register)
