#include "hw/misc/gemm_stratus.h"

#ifndef GEMM_STRATUS_DEBUG
#define GEMM_STRATUS_DEBUG 1
#endif

#define CMD_REG 0x00
#define STATUS_REG 0x04

static uint64_t gemm_stratus_mmio_read(void *opaque, hwaddr addr, unsigned size) {
    return -1;
}

static void gemm_stratus_mmio_write(void *opaque, hwaddr addr, uint64_t data, unsigned size) {

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

static void gemm_stratus_instance_init(Object *o) {
    GemmStratusState *s = GEMM_STRATUS(o);

    memory_region_init_io(&s->mmio, OBJECT(s), &gemm_stratus_mmio_ops, s, "gemm_stratus-mmio", 0x1000);
}

static void gemm_stratus_realize(DeviceState *dev, Error **errp) {
    printf("Realizing GemmStratus device\n");
}

static void gemm_stratus_class_init(ObjectClass *klass, const void *data) {
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = gemm_stratus_realize;
}

static const TypeInfo gemm_stratus_info = {
    .name = TYPE_GEMM_STRATUS,
    .parent = TYPE_DEVICE,
    .instance_size = sizeof(GemmStratusState),
    .class_init = gemm_stratus_class_init,
    .instance_init = gemm_stratus_instance_init,
};

static void gemm_stratus_register_types(void) {
    type_register_static(&gemm_stratus_info);
}

type_init(gemm_stratus_register_types)
