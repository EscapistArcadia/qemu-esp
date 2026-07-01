#include "hw/misc/esp/accelerators/template.h"
#include "hw/misc/esp/regmap.h"
#include "hw/misc/esp/sm_queue.h"
#include "hw/misc/esp/helper/dma.h"

enum Conv2dStratusMMIORegisters {
    CONV2D_N_CHANNELS_REG         = ESP_ACCELERATOR_MMIO_MAX + 0x04,
    CONV2D_N_FILTERS_REG          = ESP_ACCELERATOR_MMIO_MAX + 0x08,
    CONV2D_FILTER_HEIGHT_REG      = ESP_ACCELERATOR_MMIO_MAX + 0x0c,
    CONV2D_FILTER_WIDTH_REG       = ESP_ACCELERATOR_MMIO_MAX + 0x10,
    CONV2D_STRIDE_REG             = ESP_ACCELERATOR_MMIO_MAX + 0x14,
    CONV2D_IS_PADDED_REG          = ESP_ACCELERATOR_MMIO_MAX + 0x18,
    CONV2D_FEATURE_MAP_HEIGHT_REG = ESP_ACCELERATOR_MMIO_MAX + 0x1c,
    CONV2D_FEATURE_MAP_WIDTH_REG  = ESP_ACCELERATOR_MMIO_MAX + 0x20,
    CONV2D_DO_RELU_REG            = ESP_ACCELERATOR_MMIO_MAX + 0x24,
    CONV2D_POOL_TYPE_REG          = ESP_ACCELERATOR_MMIO_MAX + 0x28,
    CONV2D_DEPTHWISE_REG          = ESP_ACCELERATOR_MMIO_MAX + 0x2c,
    CONV2D_BATCH_SIZE_REG         = ESP_ACCELERATOR_MMIO_MAX + 0x30,
    CONV2D_INPUT_OFFSET_REG       = ESP_ACCELERATOR_MMIO_MAX + 0x34,
    CONV2D_FILTERS_OFFSET_REG     = ESP_ACCELERATOR_MMIO_MAX + 0x38,
    CONV2D_BIAS_OFFSET_REG        = ESP_ACCELERATOR_MMIO_MAX + 0x3c,
    CONV2D_OUTPUT_OFFSET_REG      = ESP_ACCELERATOR_MMIO_MAX + 0x40,
};

typedef struct Conv2dStratusConf {
    uint32_t n_channels;
    uint32_t n_filters;
    uint32_t filter_height;
    uint32_t filter_width;
    uint32_t stride;
    uint32_t is_padded;
    uint32_t feature_map_height;
    uint32_t feature_map_width;
    uint32_t do_relu;
    uint32_t pool_type;    /* 0: none, 1: 2x2 max, 2: 2x2 avg */
    uint32_t depthwise;
    uint32_t batch_size;
    uint32_t input_offset;
    uint32_t filters_offset;
    uint32_t bias_offset;
    uint32_t output_offset;
} Conv2dStratusConf;

typedef struct Conv2dStratusEntry {
    Conv2dStratusConf conv2d_params;
    sm_queue_entry_t output[MPMC_DEGREE];
} Conv2dStratusEntry;

static uint64_t conv2d_stratus_mmio_read(ESPAcceleratorState *s, hwaddr addr, unsigned size) {
    return 0;
}

static void conv2d_stratus_mmio_write(ESPAcceleratorState *s, hwaddr addr, uint64_t data, unsigned size) {
}

static void conv2d_stratus_execute(ESPAcceleratorState *s, void *param) {
    Conv2dStratusConf *p = (Conv2dStratusConf *)param;

    uint32_t n_ch   = p->n_channels;
    uint32_t n_fil  = p->n_filters;
    uint32_t fh     = p->filter_height;
    uint32_t fw     = p->filter_width;
    uint32_t stride = p->stride;
    uint32_t H      = p->feature_map_height;
    uint32_t W      = p->feature_map_width;

    /* Compute output spatial dimensions and padding */
    uint32_t out_H, out_W;
    int32_t pad_top = 0, pad_left = 0;
    if (p->is_padded) {
        out_H = (H + stride - 1) / stride;
        out_W = (W + stride - 1) / stride;
        int32_t pad_h = (int32_t)((out_H - 1) * stride + fh) - (int32_t)H;
        int32_t pad_w = (int32_t)((out_W - 1) * stride + fw) - (int32_t)W;
        pad_top  = (pad_h > 0) ? pad_h / 2 : 0;
        pad_left = (pad_w > 0) ? pad_w / 2 : 0;
    } else {
        out_H = (H - fh) / stride + 1;
        out_W = (W - fw) / stride + 1;
    }

    uint32_t pool_out_H = p->pool_type ? out_H / 2 : out_H;
    uint32_t pool_out_W = p->pool_type ? out_W / 2 : out_W;

    /* filter_size: weights per filter. depthwise has no channel dimension in weight. */
    uint32_t filter_size_base = fh * fw;
    uint32_t filter_size = p->depthwise ? filter_size_base : n_ch * filter_size_base;

    size_t sz_input   = (size_t)p->batch_size * n_ch * H * W;
    size_t sz_filters = (size_t)n_fil * filter_size;
    size_t sz_output  = (size_t)p->batch_size * n_fil * pool_out_H * pool_out_W;

    int32_t *input   = g_malloc(sz_input   * sizeof(int32_t));
    int32_t *filters = g_malloc(sz_filters * sizeof(int32_t));
    int32_t *bias    = g_malloc(n_fil       * sizeof(int32_t));
    int32_t *output  = g_malloc(sz_output  * sizeof(int32_t));
    /* Temporary buffer for one filter's pre-pool convolution output */
    int32_t *conv_tmp = p->pool_type ? g_malloc(out_H * out_W * sizeof(int32_t)) : NULL;

    uint64_t base = s->esp->sm.addr;

    dma_memory_read(&address_space_memory,
                    base + (uint64_t)p->input_offset   * sizeof(int32_t),
                    input,   sz_input   * sizeof(int32_t), MEMTXATTRS_UNSPECIFIED);
    dma_memory_read(&address_space_memory,
                    base + (uint64_t)p->filters_offset * sizeof(int32_t),
                    filters, sz_filters * sizeof(int32_t), MEMTXATTRS_UNSPECIFIED);
    dma_memory_read(&address_space_memory,
                    base + (uint64_t)p->bias_offset    * sizeof(int32_t),
                    bias,    n_fil       * sizeof(int32_t), MEMTXATTRS_UNSPECIFIED);

    /*
     * Memory layout:
     *   input   : [batch][channel][row][col]
     *   filters : [filter][channel][krow][kcol]  (depthwise: [filter][krow][kcol])
     *   bias    : [filter]
     *   output  : [batch][filter][out_row][out_col]  (after optional 2x2 pool)
     *
     * Fixed-point format: Q8.24 (32-bit signed, 8 integer bits, 24 fractional bits).
     * Multiply two Q8.24 values: (int64_t)a * b >> 24 gives a Q8.24 result.
     */
    for (uint32_t b = 0; b < p->batch_size; b++) {
        for (uint32_t fi = 0; fi < n_fil; fi++) {
            int32_t *dst_direct = &output[(b * n_fil + fi) * pool_out_H * pool_out_W];
            int32_t *dst_conv   = p->pool_type ? conv_tmp : dst_direct;

            for (uint32_t oh = 0; oh < out_H; oh++) {
                for (uint32_t ow = 0; ow < out_W; ow++) {
                    int64_t acc = 0;

                    /* depthwise: fi-th filter only applies to fi-th input channel */
                    uint32_t c_start = p->depthwise ? fi : 0;
                    uint32_t c_end   = p->depthwise ? fi + 1 : n_ch;

                    for (uint32_t c = c_start; c < c_end; c++) {
                        uint32_t c_w = p->depthwise ? 0 : c * filter_size_base;
                        for (uint32_t kh = 0; kh < fh; kh++) {
                            for (uint32_t kw = 0; kw < fw; kw++) {
                                int32_t ih = (int32_t)(oh * stride + kh) - pad_top;
                                int32_t iw = (int32_t)(ow * stride + kw) - pad_left;
                                if (ih < 0 || ih >= (int32_t)H || iw < 0 || iw >= (int32_t)W) {
                                    continue;
                                }
                                int32_t in_val = input[b * n_ch * H * W + c * H * W + ih * W + iw];
                                int32_t w_val  = filters[fi * filter_size + c_w + kh * fw + kw];
                                acc += ((int64_t)in_val * w_val) >> 24;
                            }
                        }
                    }

                    acc += (int64_t)bias[fi];
                    if (p->do_relu && acc < 0) {
                        acc = 0;
                    }
                    dst_conv[oh * out_W + ow] = (int32_t)acc;
                }
            }

            /* Apply 2x2 pooling over conv_tmp into the final output */
            if (p->pool_type) {
                for (uint32_t ph = 0; ph < pool_out_H; ph++) {
                    for (uint32_t pw = 0; pw < pool_out_W; pw++) {
                        int32_t a = conv_tmp[(2 * ph)     * out_W + (2 * pw)    ];
                        int32_t b2= conv_tmp[(2 * ph)     * out_W + (2 * pw + 1)];
                        int32_t c = conv_tmp[(2 * ph + 1) * out_W + (2 * pw)    ];
                        int32_t d = conv_tmp[(2 * ph + 1) * out_W + (2 * pw + 1)];
                        int32_t res;
                        if (p->pool_type == 1) {
                            /* max pool */
                            res = a;
                            if (b2 > res) res = b2;
                            if (c  > res) res = c;
                            if (d  > res) res = d;
                        } else {
                            /* average pool: shift right 2 divides by 4 in Q8.24 */
                            res = (int32_t)(((int64_t)a + b2 + c + d) >> 2);
                        }
                        dst_direct[ph * pool_out_W + pw] = res;
                    }
                }
            }
        }
    }

    dma_memory_write(&address_space_memory,
                     base + (uint64_t)p->output_offset * sizeof(int32_t),
                     output, sz_output * sizeof(int32_t), MEMTXATTRS_UNSPECIFIED);

    g_free(input);
    g_free(filters);
    g_free(bias);
    g_free(output);
    g_free(conv_tmp);
}

ESPAccelerator conv2d_stratus_rr = {
    .type = "conv2d_stratus",

    .conf_size = sizeof(Conv2dStratusConf),

    .mmio_read  = conv2d_stratus_mmio_read,
    .mmio_write = conv2d_stratus_mmio_write,

    .pick_context = pick_context_rr,
    .execute      = conv2d_stratus_execute,
};

ESPAccelerator conv2d_stratus_fair = {
    .type = "conv2d_stratus",

    .conf_size = sizeof(Conv2dStratusConf),

    .mmio_read  = conv2d_stratus_mmio_read,
    .mmio_write = conv2d_stratus_mmio_write,

    .pick_context = pick_context_fair,
    .execute      = conv2d_stratus_execute,
};

static void conv2d_stratus_register(void) {
    /* TODO: check configuration, rr by default */
    esp_accelerator_type_register(&conv2d_stratus_rr);
}

type_init(conv2d_stratus_register)
