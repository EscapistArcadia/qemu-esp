#include "hw/misc/gemm_stratus.h"
#include "hw/core/sysbus.h"
#include "qapi/error.h"

#ifndef GEMM_STRATUS_DEBUG
#define GEMM_STRATUS_DEBUG 1
#endif

DeviceState *gemm_stratus_create(void) {
    DeviceState *dev = qdev_new(TYPE_GEMM_STRATUS);
    SysBusDevice *sbdev = SYS_BUS_DEVICE(dev);
    
    sysbus_realize(sbdev, &error_fatal);

    // sysbus_connect_irq(sbdev, 0, GEMM_STRATUS(dev)->irq);
    // TODO: check the necessity of the above line

    return dev;
}

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
    SysBusDeviceClass *sdc = SYS_BUS_DEVICE_CLASS(klass);

    sdc->parent_class.realize = gemm_stratus_realize;
}

static const TypeInfo gemm_stratus_info = {
    .name = TYPE_GEMM_STRATUS,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(GemmStratusState),
    .class_init = gemm_stratus_class_init,
    .instance_init = gemm_stratus_instance_init,
};

static void gemm_stratus_register_types(void) {
    type_register_static(&gemm_stratus_info);
}

type_init(gemm_stratus_register_types)
