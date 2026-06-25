#include "hw/misc/esp/accelerator.h"

#include "hw/misc/esp/helper/int.h"
#include "hw/misc/esp/helper/dma.h"

#include "hw/misc/esp/accelerators/gemm_stratus.h"

#define ESP_ACCELERATOR_MMIO_NAME "esp-accelerator-mmio"
#define ESP_ACCELERATOR_MMIO_BASE (0x60010000)
#define ESP_ACCELERATOR_MMIO_SIZE (0x100)

#if 1
#include "accelerator_mmio_verbose.inc"
#else
#define esp_accelerator_mmio_read_verbose(addr, size)
#define esp_accelerator_mmio_write_verbose(addr, content, size)
#endif

#define IS_VALID_CONTEXT(bitmap, ctxt) (((bitmap) & (1 << (ctxt))) != 0)

static void *esp_accelerator_execute(void *opaque) {
    ESPAcceleratorState *s = opaque;

    uint64_t pt_base, input_queue_base;
    uint32_t next_context;
    GemmStratusEntry gemm_params; /* TODO: generalize the configuration */

    while (1) {
        /* TODO: replace with the check_next_context logic from the real hardware implementation */
        for (next_context = 0; next_context < MAX_CONTEXTS; ++next_context) {
            if (IS_VALID_CONTEXT(s->valid_contexts, next_context)) {
                pt_base = dma_read(MAKEDWORD(s->pt_address_low[next_context], s->pt_address_high), 0, uint32_t);
                input_queue_base = pt_base + s->queue_ptr[next_context] * sizeof(uint32_t);
                
                if (!sm_queue_can_pop(s, input_queue_base, &gemm_params, sizeof(GemmStratusEntry))) {
                    continue;
                }
                
                // output_queue_base = pt_base + gemm_params.output[0].output_queue * sizeof(uint32_t);
                if (!sm_queue_can_push(s, s->sm_info_output_queue[0])) {
                    continue;
                }

                printf("[QEMU] Processing context %u with parameters: ninputs=%u, d1=%u, d2=%u, d3=%u, ld_offset1=%u, ld_offset2=%u, bias_offset=%u, st_offset=%u, do_relu=%u, do_bias=%u, transpose=%u, output[0].output_queue=0x%x, output[0].output_entry=0x%x, output[1].output_queue=0x%x, output[1].output_entry=0x%x\n",
                    next_context,
                    gemm_params.gemm_params.ninputs,
                    gemm_params.gemm_params.d1,
                    gemm_params.gemm_params.d2,
                    gemm_params.gemm_params.d3,
                    gemm_params.gemm_params.ld_offset1,
                    gemm_params.gemm_params.ld_offset2,
                    gemm_params.gemm_params.bias_offset,
                    gemm_params.gemm_params.st_offset,
                    gemm_params.gemm_params.do_relu,
                    gemm_params.gemm_params.do_bias,
                    gemm_params.gemm_params.transpose,
                    gemm_params.output[0].output_queue,
                    gemm_params.output[0].output_entry,
                    gemm_params.output[1].output_queue,
                    gemm_params.output[1].output_entry);

                /* execute the GEMM operation */

                sm_queue_pop(s, input_queue_base);
                sm_queue_push(s, s->sm_info_output_queue[0], s->sm_info_output_entry[0]);
            }
        }
    }

    return NULL;
}

static uint64_t esp_accelerator_mmio_read(void *opaque, hwaddr addr, unsigned size) {
    ESPAcceleratorState *s = opaque;

    esp_accelerator_mmio_read_verbose(addr, size);

    switch (addr) {
        case PT_ADDRESS_REG_0:
        case PT_ADDRESS_REG_1:
        case PT_ADDRESS_REG_2:
        case PT_ADDRESS_REG_3: {
        uint64_t context_id = (addr - PT_ADDRESS_REG_0) / 4;
        return s->pt_address_low[context_id];
        }
case PT_ADDRESS_EXTENDED_REG: {
        return s->pt_address_high;
        }
case AMU_INFO_NPRIO_REG_0:
        case AMU_INFO_NPRIO_REG_1:
        case AMU_INFO_NPRIO_REG_2:
        case AMU_INFO_NPRIO_REG_3: {
        uint64_t context_id = (addr - AMU_INFO_NPRIO_REG_0) / 4;
        return s->nprio[context_id];
        }
case AMU_INFO_VLD_CTXT_REG: {
        return s->valid_contexts;
        }
case AMU_INFO_SCHED_PERIOD_REG: {
        return s->sched_period;
        }
case AMU_INFO_QUEUE_PTR_REG_0:
        case AMU_INFO_QUEUE_PTR_REG_1:
        case AMU_INFO_QUEUE_PTR_REG_2:
        case AMU_INFO_QUEUE_PTR_REG_3: {
        uint64_t context_id = (addr - AMU_INFO_QUEUE_PTR_REG_0) / 4;
        return s->queue_ptr[context_id];
        }
    }

    return 0;
}

static void esp_accelerator_mmio_write(void *opaque, hwaddr addr, uint64_t data, unsigned size) {
    ESPAcceleratorState *s = opaque;

    esp_accelerator_mmio_write_verbose(addr, data, size);

    switch (addr) {
        case PT_ADDRESS_REG_0:
        case PT_ADDRESS_REG_1:
        case PT_ADDRESS_REG_2:
        case PT_ADDRESS_REG_3: {
            uint64_t context_id = (addr - PT_ADDRESS_REG_0) / 4;
            s->pt_address_low[context_id] = LOWORD(data);
            break;
        }
        case PT_ADDRESS_EXTENDED_REG: {
            s->pt_address_high = LOWORD(data);
            break;
        }
        case AMU_INFO_NPRIO_REG_0:
        case AMU_INFO_NPRIO_REG_1:
        case AMU_INFO_NPRIO_REG_2:
        case AMU_INFO_NPRIO_REG_3: {
            uint64_t context_id = (addr - AMU_INFO_NPRIO_REG_0) / 4;
            s->nprio[context_id] = LOWORD(data);
            break;
        }
        case AMU_INFO_VLD_CTXT_REG: {
            s->valid_contexts = LOWORD(data);
            break;
        }
        case AMU_INFO_SCHED_PERIOD_REG: {
            s->sched_period = LOWORD(data);
            break;
        }
        case AMU_INFO_QUEUE_PTR_REG_0:
        case AMU_INFO_QUEUE_PTR_REG_1:
        case AMU_INFO_QUEUE_PTR_REG_2:
        case AMU_INFO_QUEUE_PTR_REG_3: {
            uint64_t context_id = (addr - AMU_INFO_QUEUE_PTR_REG_0) / 4;
            s->queue_ptr[context_id] = LOWORD(data);
            break;
        }
    }
}

static const MemoryRegionOps esp_accelerator_mmio_ops = {
    .read = esp_accelerator_mmio_read,
    .write = esp_accelerator_mmio_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    }
};

#define INIT_MMIO_REGION(mr, name, base, size) \
    do { \
        sysbus_init_mmio(SYS_BUS_DEVICE(s), &mr); \
        memory_region_init_io(&mr, OBJECT(s), &esp_accelerator_mmio_ops, s, name, size); \
        memory_region_add_subregion(get_system_memory(), base, &mr); \
    } while (0)

DeviceState *esp_accelerator_create(ESPSubsystemState *esp, const char *type, hwaddr mmio_base, uint64_t mmio_size) {
    DeviceState *dev = qdev_new(type);
    ESPAcceleratorState *s = ESP_ACCELERATOR(dev);

    s->esp = esp;

    /* TODO: generate unique name */
    INIT_MMIO_REGION(s->mmio, "esp_accelerator", mmio_base, mmio_size);

    /* TODO: add return value check */
    qemu_thread_create(&s->execution, type, esp_accelerator_execute, s, QEMU_THREAD_DETACHED);

    /* TODO: create other threads */

    return dev;
}

static const TypeInfo esp_accelerator_info = {
    .name = TYPE_ESP_ACCELERATOR,
    /**
     * You know, I even found that this has been wrong for a long time just after I realized that this is actually correct. There should be no inheritance relationship between the accelerator and the subsystem. Otherwise, every time the accelerator is created, the subsystem will be created again, which is not what I desired.
     */
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(ESPAcceleratorState),

    // .class_init = esp_accelerator_class_init,
};

static void esp_accelerator_register_types(void) {
    type_register_static(&esp_accelerator_info);
}

type_init(esp_accelerator_register_types)
