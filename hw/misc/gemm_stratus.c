#include "hw/misc/gemm_stratus.h"
#include "hw/core/sysbus.h"
#include "system/address-spaces.h"
#include "qapi/error.h"

#ifndef GEMM_STRATUS_DEBUG
#define GEMM_STRATUS_DEBUG 1
#endif

#ifndef ESP_ACCEL_REG

/***
 * ESP accelerator common definitions and registers offset
 ***/

/* bank(0): CMD (reset if cleared) */
#define CMD_REG 0x00
#define CMD_MASK_START BIT(0)

/* bank(1): STATUS (idle when cleared) - Read only */
#define STATUS_REG 0x04
#define STATUS_MASK_RUN  BIT(0)
#define STATUS_MASK_DONE BIT(1)
#define STATUS_MASK_ERR  BIT(2)

/* bank(2)        : SELECT (which accelerator will run in 1 hot encoding) */
#define SELECT_REG 0x08

/* bank(3)        : RESERVED - Read only */
#define DEVID_REG 0x0c

/* bank(4)        : PT_ADDRESS (page table bus address) */
#define PT_ADDRESS_REG 0x10

/* bank(5)        : PT_NCHUNK (number of physical contiguous buffers in memory) */
#define PT_NCHUNK_REG 0x14

/* bank(6)        : PT_SHIFT (log2(cunk size)) */
#define PT_SHIFT_REG 0x18

/* bank(7)        : PT_NCHUNK_MAX (maximum number of chunks supported) - Read only */
#define PT_NCHUNK_MAX_REG 0x1c

/* bank(8)        : PT_ADDRESS_EXTENDED (page table bus address MSBs) */
#define PT_ADDRESS_EXTENDED_REG 0x20

/* bank(9)        : Type of coherence (None, LLC, Full) - Read only */
#define COHERENCE_REG 0x24
enum accelerator_coherence {ACC_COH_NONE = 0, ACC_COH_LLC, ACC_COH_RECALL, ACC_COH_FULL, ACC_COH_AUTO};

/* bank(10)       : Point-to-point configuration */
#define P2P_REG 0x28
#define P2P_MASK_NSRCS 0x3
#define P2P_MASK_SRC_IS_P2P BIT(2)
#define P2P_MASK_DST_IS_P2P BIT(3)
#define P2P_MASK_SRCS_YX 0x7
#define P2P_SHIFT_SRCS_Y(_n) (7 + _n * 6)
#define P2P_SHIFT_SRCS_X(_n) (4 + _n * 6)

/* bank(11)       : RESERVED */
#define YX_REG 0x2c
#define YX_SHIFT_X 0
#define YX_SHIFT_Y 16
#define YX_MASK_YX 0x7

/* bank(12)       : SRC_OFFSET (offset in bytes from beginning of physical buffer) */
#define SRC_OFFSET_REG 0x30

/* bank(13)       : DST_OFFSET (offset in bytes from beginning of physical buffer) */
#define DST_OFFSET_REG 0x34

/* bank(14)       : SPANDEX_REG */
#define SPANDEX_REG 0x38

/* bank(15)       : VALID_CONTEXTS_ACK_REG */
#define VALID_CONTEXTS_ACK_REG 0x3c

/* bank(16 to 19) : PT_ADDRESS_REG_i */
#define PT_ADDRESS_REG_0 0x40
#define PT_ADDRESS_REG_1 0x44
#define PT_ADDRESS_REG_2 0x48
#define PT_ADDRESS_REG_3 0x4c

/* bank(20 to 27) : MON_UTIL_REG_i */
#define MON_UTIL_REG_0_LO 0x50
#define MON_UTIL_REG_0_HI 0x54
#define MON_UTIL_REG_1_LO 0x58
#define MON_UTIL_REG_1_HI 0x5c
#define MON_UTIL_REG_2_LO 0x60
#define MON_UTIL_REG_2_HI 0x64
#define MON_UTIL_REG_3_LO 0x68
#define MON_UTIL_REG_3_HI 0x6c

/* bank(28 to 63) : USR (user defined) */

// Re-enable the following 3 registers if adding an SRAM expanding the register bank
// /* bank(29)       : EXP_ADDR (bits 29:0 address an SRAM expanding the register bank) */
// #define EXP_ADDR_REG 0x74
// #define EXT_MASK_R BIT(30)
// #define EXT_MASK_W BIT(31)
// /* bank(30)       : EXP_DI (data to be written to the expansion SRAM) */
// #define EXP_DI_REG 0x78
// /* bank(31)       : EXP_DO (data read from the exansion SRAM) */
// #define EXP_DO_REG 0x7c

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
    return 0;
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

static void gemm_stratus_realize(DeviceState *dev, Error **errp) {
    GemmStratusState *s = GEMM_STRATUS(dev);

    memory_region_init_io(&s->mmio, OBJECT(s), &gemm_stratus_mmio_ops, s, "gemm_stratus-mmio", GEMM_MMIO_SIZE);
    memory_region_add_subregion(get_system_memory(), GEMM_MMIO_BASE, &s->mmio);
    sysbus_init_mmio(SYS_BUS_DEVICE(s), &s->mmio);
        

    sysbus_init_irq(SYS_BUS_DEVICE(s), &s->irq);
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
