#include "hw/misc/esp/subsystem.h"

#include <libfdt.h>

#include "hw/misc/esp/accelerator.h"
#include "hw/misc/esp/helper/int.h"
#include "system/address-spaces.h"
#include "qapi/error.h"
#include "qemu/error-report.h" /* TODO: refactor include to only include high-level header files */

#define ESP_ACCELERATOR_RESERVED "accelerator-reserved"

#define ESP_GRETH_RESERVED "greth_reserved"

#define ESP_FDT_RESERVED_MEMORY "reserved-memory"

#define INIT_RAM_REGION(mr, name, base, size) \
    do { \
        memory_region_init_ram(&mr, OBJECT(s), name, size, &error_fatal); \
        memory_region_add_subregion(get_system_memory(), base, &mr); \
    } while (0)

static int fdt_get_prop_symbol(const void *fdt, int symbol, const char *name) {
    int prop_path_len;
    const char *prop_path = fdt_getprop(fdt, symbol, name, &prop_path_len);
    if (!prop_path) {
        error_report("Property %s not found in the device tree", name);
        exit(1);
    }

    return fdt_path_offset(fdt, prop_path);
}

static void fdt_parse_region(const void *fdt, int offset, hwaddr *base, uint64_t *size) {
    int reg_len;
    const fdt32_t *reg = fdt_getprop(fdt, offset, "reg", &reg_len);
    if (!reg) {
        error_report("Property 'reg' not found in the device tree node at offset %d", offset);
        exit(1);
    }

    if (reg_len < 2 * sizeof(fdt32_t)) {
        error_report("Property 'reg' is too short in the device tree node at offset %d", offset);
        exit(1);
    }

    *base = MAKEDWORD(fdt32_to_cpu(reg[1]), fdt32_to_cpu(reg[0])); /* TODO: more flexible, and remove mem access (fdt_address_cells, fdt_size_cells) */
    *size = MAKEDWORD(fdt32_to_cpu(reg[3]), fdt32_to_cpu(reg[2]));
}

DeviceState *esp_subsystem_init(void *fdt) {
    DeviceState *dev = qdev_new(TYPE_ESP_SUBSYSTEM);
    ESPSubsystemState *s = ESP_SUBSYSTEM(dev);

    int symbol_offset, reserved_offset, greth_offset, accel_offset, soc_offset;
    // int address_cells, size_cells; /* TODO: do we need to parse these? */
    ESPAccelerator *accel;
    const char *accel_name, *at_pos;
    hwaddr mr_base;
    uint64_t mr_size;

    QLIST_INIT(&s->accelerators);

    symbol_offset = fdt_path_offset(fdt, "/__symbols__"); /* TODO: any better ways to skip symbols? */
    if (symbol_offset < 0) {
        error_report("Symbols node not found in the device tree, please regenerate dtb with -@ option");
        exit(1);
    }

    reserved_offset = fdt_path_offset(fdt, "/reserved-memory");
    if (reserved_offset < 0) {
        exit(1);
    }

    greth_offset = fdt_get_prop_symbol(fdt, symbol_offset, "greth_reserved");
    fdt_parse_region(fdt, greth_offset, &mr_base, &mr_size);
    INIT_RAM_REGION(s->greth, ESP_GRETH_RESERVED, mr_base, mr_size);

    accel_offset = fdt_get_prop_symbol(fdt, symbol_offset, "accelerator_reserved");
    fdt_parse_region(fdt, accel_offset, &mr_base, &mr_size);
    INIT_RAM_REGION(s->sm, ESP_ACCELERATOR_RESERVED, mr_base, mr_size);

    soc_offset = fdt_get_prop_symbol(fdt, symbol_offset, "L26"); /* TODO: what does L26 mean? check socgen.py of ESP */

    for (soc_offset = fdt_first_subnode(fdt, soc_offset); soc_offset >= 0; soc_offset = fdt_next_subnode(fdt, soc_offset)) {
        accel_name = fdt_get_name(fdt, soc_offset, NULL);
        at_pos = strchr(accel_name, '@');
        if (at_pos) { /* TODO: find a more elegant way to handle this */
            accel_name = strndup(accel_name, at_pos - accel_name);
        }

        accel = esp_accelerator_type(accel_name);
        if (accel) {
            fdt_parse_region(fdt, soc_offset, &mr_base, &mr_size);
            esp_accelerator_create(s, accel, mr_base, mr_size);
        }

        if (at_pos) {
            free((void *)accel_name);
        }
    }

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
