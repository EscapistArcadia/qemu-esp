#include "hw/misc/gemm_stratus.h"
#include "exec/memattrs.h"
#include "hw/core/sysbus.h"
#include "system/address-spaces.h"
#include "qapi/error.h"
#include "system/dma.h"


#ifndef GEMM_STRATUS_DEBUG
#define GEMM_STRATUS_DEBUG 1
#endif

#ifndef ESP_ACCEL_REG

enum GemmStratusRegisters {
    CMD_REG = 0x00,
    STATUS_REG = 0x04,
    SELECT_REG = 0x08,
    DEVID_REG = 0x0c,
    PT_ADDRESS_REG = 0x10,
    PT_NCHUNK_REG = 0x14,
    PT_SHIFT_REG = 0x18,
    PT_NCHUNK_MAX_REG = 0x1c,
    PT_ADDRESS_EXTENDED_REG = 0x20,
    PT_COHERENCE_REG = 0x24,
    P2P_REG = 0x28,
    YX_REG = 0x2c,
    SRC_OFFSET_REG = 0x30,
    DST_OFFSET_REG = 0x34,
    ESP_MAX = DST_OFFSET_REG,

    /* following: vignesh's fantastic additions */
    SPANDEX_REG = 0x38,

    VALID_CONTEXTS_ACK_REG = 0x3c,
    
    PT_ADDRESS_REG_0 = 0x40,
    PT_ADDRESS_REG_1 = 0x44,
    PT_ADDRESS_REG_2 = 0x48,
    PT_ADDRESS_REG_3 = 0x4c,

    MON_UTIL_REG_0_LO = 0x50,
    MON_UTIL_REG_0_HI = 0x54,
    MON_UTIL_REG_1_LO = 0x58,
    MON_UTIL_REG_1_HI = 0x5c,
    MON_UTIL_REG_2_LO = 0x60,
    MON_UTIL_REG_2_HI = 0x64,
    MON_UTIL_REG_3_LO = 0x68,
    MON_UTIL_REG_3_HI = 0x6c,

    AMU_INFO_QUEUE_PTR_REG_0 = 0x70,
    AMU_INFO_QUEUE_PTR_REG_1 = 0x74,
    AMU_INFO_QUEUE_PTR_REG_2 = 0x78,
    AMU_INFO_QUEUE_PTR_REG_3 = 0x7c,

    AMU_INFO_NPRIO_REG_0 = 0x80,
    AMU_INFO_NPRIO_REG_1 = 0x84,
    AMU_INFO_NPRIO_REG_2 = 0x88,
    AMU_INFO_NPRIO_REG_3 = 0x8c,

    AMU_INFO_VLD_CTXT_REG = 0x90,
    AMU_INFO_SCHED_PERIOD_REG = 0x94,

    /* following: gemm */
    GEMM_NINPUTS_REG = 0x98,
    GEMM_D1_REG = 0x9c,
    GEMM_D2_REG = 0xa0,
    GEMM_D3_REG = 0xa4,
    GEMM_LD_OFFSET1_REG = 0xa8,
    GEMM_LD_OFFSET2_REG = 0xac,
    GEMM_BIAS_OFFSET_REG = 0xb0,
    GEMM_ST_OFFSET_REG = 0xb4,
    GEMM_DO_RELU_REG = 0xb8,
    GEMM_DO_BIAS_REG = 0xbc,
    GEMM_TRANSPOSE_REG = 0xc0,
};

#endif

MemoryRegion greth_reserved;
MemoryRegion accel_reserved;

DeviceState *gemm_stratus_create(void) {
    DeviceState *dev = qdev_new(TYPE_GEMM_STRATUS);
    SysBusDevice *sbdev = SYS_BUS_DEVICE(dev);
    
    sysbus_realize(sbdev, &error_fatal);

    // sysbus_connect_irq(sbdev, 0, GEMM_STRATUS(dev)->irq);
    // TODO: check the necessity of the above line

    return dev;
}

#if 1
#include "hw/misc/gemm_stratus_mmio.inc"
#else
#define gemm_stratus_mmio_read_verbose(addr, size)
#define gemm_stratus_mmio_write_verbose(addr, data, size)
#endif

static uint64_t gemm_stratus_mmio_read(void *opaque, hwaddr addr, unsigned size) {
    gemm_stratus_mmio_read_verbose(addr, size);

    return 0;
}

#define MAKEDWORD(lo, hi) (((uint64_t)(hi) << 32) | (uint64_t)(lo))

#define LOWORD(x) ((uint32_t)((x) & 0xffffffff))
#define HIWORD(x) ((uint32_t)(((x) >> 32) & 0xffffffff))

/*
#define UPDATE_LO(dest, src) \
    do { \
        (dest) |= ((uint64_t)src & 0xffffffff); \
    } while (0)
#define UPDATE_HI(dest, src) \
    do { \
        (dest) |= ((uint64_t)src << 32); \
    } while (0)
*/

static void gemm_stratus_mmio_write(void *opaque, hwaddr addr, uint64_t data, unsigned size) {
    GemmStratusState *s = opaque;

    gemm_stratus_mmio_write_verbose(addr, data, size);

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

static const MemoryRegionOps gemm_stratus_mmio_ops = {
    .read = gemm_stratus_mmio_read,
    .write = gemm_stratus_mmio_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    }
};

typedef struct sm_queue_slot_t {
    uint64_t seq;
    uint64_t entry;
} sm_queue_slot_t;

typedef struct sm_queue_t {
    uint64_t stat;
    uint64_t head;
    uint64_t tail;
    uint64_t next_queue_ptr;
    sm_queue_slot_t slots[CONTEXT_COUNT];
} sm_queue_t;

static void *gemm_stratus(void *arg) {
    GemmStratusState *s = (GemmStratusState *)arg;

    uint64_t pt_base, pt_address, queue_base;
    // sm_queue_t queue[CONTEXT_COUNT];
    sm_queue_t queue;

    while (1) {
        /* verbose queue data every 5 seconds */
        for (int ctx = 0; ctx < CONTEXT_COUNT; ctx++) {
            pt_address = MAKEDWORD(s->pt_address_low[ctx], s->pt_address_high);
            dma_memory_read(&address_space_memory, pt_address, &pt_base, sizeof(uint64_t), MEMTXATTRS_UNSPECIFIED);

            queue_base = (pt_base + (s->queue_ptr[ctx] * 4));
            if (queue_base) {
                dma_memory_read(&address_space_memory, queue_base, &queue, sizeof(sm_queue_t), MEMTXATTRS_UNSPECIFIED);
                printf("queue[%d]: stat=0x%lx, head=0x%lx, tail=0x%lx, next_queue_ptr=0x%lx\n", ctx, queue.stat, queue.head, queue.tail, queue.next_queue_ptr);
                for (int i = 0; i < CONTEXT_COUNT; i++) {
                    printf("  slot[%d]: seq=0x%lx, entry=0x%lx\n", i, queue.slots[i].seq, queue.slots[i].entry);
                }
            } else {
                printf("queue[%d]: NULL\n", ctx);
            }
        }

        g_usleep(5000000);
    }

    return NULL;
}

/**
 * @brief Temporary wrapper for initializing a subreagion of main memory chip as the reserved memory for special purposes
 * 
 * @param name MemoryRegion
 * @param base Base address of the reserved memory region
 * @param size Size of the reserved memory region
 */
#define INIT_RAM_REGION(name, base, size) \
    do { \
        sysbus_init_mmio(SYS_BUS_DEVICE(s), &name); \
        memory_region_init_ram(&name, OBJECT(s), #name, size, &error_fatal); \
        memory_region_add_subregion(get_system_memory(), base, &name); \
    } while (0)

/**
 * @brief Temporary wrapper for initializing the MMIO region for accelerators and adding it to the system memory map
 * 
 * @param name MemoryRegion
 * @param base Base address of the MMIO region
 * @param size Size of the MMIO region
 */
#define INIT_MMIO_REGION(name, base, size) \
    do { \
        sysbus_init_mmio(SYS_BUS_DEVICE(s), &name); \
        memory_region_init_io(&name, OBJECT(s), &gemm_stratus_mmio_ops, s, #name, size); \
        memory_region_add_subregion(get_system_memory(), base, &name); \
    } while (0)

static void gemm_stratus_realize(DeviceState *dev, Error **errp) {
    GemmStratusState *s = GEMM_STRATUS(dev);

    /* TODO: Substitute "magic" number with constants */
    /* TODO: Then, parse the device tree to obtain the base addresses and sizes */
    INIT_RAM_REGION(greth_reserved, 0xa0000000, 0x00200000);
    INIT_RAM_REGION(accel_reserved, 0xa0200000, 0x1fe00000);

    /* TODO: Proper name for the MMIO region, rather than s->mmio */
    INIT_MMIO_REGION(s->mmio, GEMM_MMIO_BASE, GEMM_MMIO_SIZE); 

    sysbus_init_irq(SYS_BUS_DEVICE(s), &s->irq);

    qemu_thread_create(&s->gemm_eq, TYPE_GEMM_STRATUS, gemm_stratus, s, QEMU_THREAD_DETACHED);
}

static void gemm_stratus_class_init(ObjectClass *klass, const void *data) {
    SysBusDeviceClass *sdc = SYS_BUS_DEVICE_CLASS(klass);

    sdc->parent_class.realize = gemm_stratus_realize;
}

static const TypeInfo gemm_stratus_info = {
    .name = TYPE_GEMM_STRATUS,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(GemmStratusState),
    .class_init = gemm_stratus_class_init,
    // .instance_init = gemm_stratus_instance_init,
};

static void gemm_stratus_register_types(void) {
    type_register_static(&gemm_stratus_info);
}

type_init(gemm_stratus_register_types)
