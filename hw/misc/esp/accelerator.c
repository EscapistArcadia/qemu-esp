#include "hw/misc/esp/accelerator.h"

#include "system/address-spaces.h"
#include "system/dma.h"

#include "hw/misc/esp/helper/int.h"
#include "hw/misc/esp/helper/dma.h"

#define ESP_ACCELERATOR_MMIO_NAME "esp-accelerator-mmio"
#define ESP_ACCELERATOR_MMIO_BASE (0x60010000)
#define ESP_ACCELERATOR_MMIO_SIZE (0x100)

#define ESP_ACCELERATOR_MMIO_READ_VERBOSE(name, size) \
    case name: \
        printf("[QEMU] MMIO READ 0x%08"HWADDR_PRIx" (%s)\n", addr, #name); \
        break;

#define ESP_ACCELERATOR_MMIO_WRITE_VERBOSE(name, content, size) \
    case name: \
        printf("[QEMU] MMIO WRITE 0x%08"HWADDR_PRIx" (%s) = 0x%016"PRIx64"\n", addr, #name, content); \
        break;

#if 1
#include "accelerator_mmio_verbose.inc"
#else
#define esp_accelerator_mmio_read_verbose(addr, size)
#define esp_accelerator_mmio_write_verbose(addr, content, size)
#endif

#define IS_VALID_CONTEXT(bitmap, ctxt) (((bitmap) & (1 << (ctxt))) != 0)

static void *esp_accelerator_execute(void *opaque) {
    ESPAcceleratorState *s = opaque;

    uint64_t pt_base, pt_address, queue_base;
    uint32_t next_context;

    uint64_t tail, seq;

    while (1) {
        for (next_context = 0; next_context < MAX_CONTEXTS; ++next_context) {
            if (IS_VALID_CONTEXT(s->valid_contexts, next_context)) {
                pt_address = MAKEDWORD(s->pt_address_low[next_context], s->pt_address_high);
                pt_base = dma_read(pt_address, 0, uint32_t);
                queue_base = pt_base + s->queue_ptr[next_context] * sizeof(uint32_t);

                tail = dma_read(queue_base, offsetof(sm_queue_t, tail), uint64_t) % SM_QUEUE_SIZE;
                seq = dma_read(queue_base, offsetof(sm_queue_t, slot[tail]), uint64_t);

                if (seq == (tail + 1)) {
                    printf("[QEMU] Processing context %u with parameters: pt_address=0x%016"PRIx64", queue_base=0x%016"PRIx64"\n",
                        next_context, pt_address, queue_base);
                    g_usleep(1000000);
                    break;
                }
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

DeviceState *esp_accelerator_create(const char *type, hwaddr mmio_base, uint64_t mmio_size) {
    DeviceState *dev = qdev_new(type);
    ESPAcceleratorState *s = ESP_ACCELERATOR(dev);

    /* TODO: generate unique name */
    INIT_MMIO_REGION(s->mmio, "esp_accelerator", mmio_base, mmio_size);

    /* TODO: add return value check */
    qemu_thread_create(&s->execution, type, esp_accelerator_execute, s, QEMU_THREAD_DETACHED);

    /* TODO: create other threads */

    return dev;
}

static const TypeInfo esp_accelerator_info = {
    .name = TYPE_ESP_ACCELERATOR,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(ESPAcceleratorState),

    // .class_init = esp_accelerator_class_init,
};

static void esp_accelerator_register_types(void) {
    type_register_static(&esp_accelerator_info);
}

type_init(esp_accelerator_register_types)
