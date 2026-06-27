#include "hw/misc/esp/subsystem.h"

#include "hw/misc/esp/accelerator.h"
#include "system/address-spaces.h"
#include "qapi/error.h"

#define ESP_SHARED_MEMORY_NAME "esp-subsystem-reserved"
#define ESP_SHARED_MEMORY_BASE (0xa0200000)
#define ESP_SHARED_MEMORY_SIZE (0x1fe00000)

#define ESP_GRETH_RESERVED_NAME "esp-greth-reserved"
#define ESP_GRETH_RESERVED_BASE (0xa0000000)
#define ESP_GRETH_RESERVED_SIZE (0x00200000)

/* TODO: automate the MMIO base and size by reading from the device tree */
#define GEMM_STRATUS_MMIO_BASE (0x60010000)
#define GEMM_STRATUS_MMIO_SIZE (0x100)

#define INIT_RAM_REGION(mr, name, base, size) \
    do { \
        sysbus_init_mmio(SYS_BUS_DEVICE(s), &mr); \
        memory_region_init_ram(&mr, OBJECT(s), name, size, &error_fatal); \
        memory_region_add_subregion(get_system_memory(), base, &mr); \
    } while (0)

DeviceState *esp_subsystem_init(void) {
    DeviceState *dev = qdev_new(TYPE_ESP_SUBSYSTEM);
    ESPSubsystemState *s = ESP_SUBSYSTEM(dev);

    INIT_RAM_REGION(s->sm, ESP_SHARED_MEMORY_NAME, ESP_SHARED_MEMORY_BASE, ESP_SHARED_MEMORY_SIZE);
    INIT_RAM_REGION(s->greth, ESP_GRETH_RESERVED_NAME, ESP_GRETH_RESERVED_BASE, ESP_GRETH_RESERVED_SIZE);

    /* TODO: automatically create all accelerators from the device tree */
    esp_accelerator_create(s, NULL, GEMM_STRATUS_MMIO_BASE, GEMM_STRATUS_MMIO_SIZE);
    esp_accelerator_create(s, NULL, GEMM_STRATUS_MMIO_BASE + GEMM_STRATUS_MMIO_SIZE, GEMM_STRATUS_MMIO_SIZE);
    esp_accelerator_create(s, NULL, GEMM_STRATUS_MMIO_BASE + 2 * GEMM_STRATUS_MMIO_SIZE, GEMM_STRATUS_MMIO_SIZE);
    esp_accelerator_create(s, NULL, GEMM_STRATUS_MMIO_BASE + 3 * GEMM_STRATUS_MMIO_SIZE, GEMM_STRATUS_MMIO_SIZE);
    esp_accelerator_create(s, NULL, GEMM_STRATUS_MMIO_BASE + 4 * GEMM_STRATUS_MMIO_SIZE, GEMM_STRATUS_MMIO_SIZE);
    esp_accelerator_create(s, NULL, GEMM_STRATUS_MMIO_BASE + 5 * GEMM_STRATUS_MMIO_SIZE, GEMM_STRATUS_MMIO_SIZE);

    return dev;
}

static const TypeInfo esp_subsystem_info = {
    .name = TYPE_ESP_SUBSYSTEM,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(ESPSubsystemState),
};

static void esp_subsystem_register_types(void) {
    type_register_static(&esp_subsystem_info);
}

type_init(esp_subsystem_register_types)
