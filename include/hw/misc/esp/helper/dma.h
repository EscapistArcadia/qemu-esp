#ifndef ESP_HELPER_DMA_H
#define ESP_HELPER_DMA_H

#include "qemu/osdep.h"
#include "system/address-spaces.h"
#include "system/dma.h"

#define dma_read(base, offset, type) \
    ({ \
        type vvs; \
        dma_memory_read(&address_space_memory, base + offset, &vvs, sizeof(type), MEMTXATTRS_UNSPECIFIED); \
        vvs; \
    })

#define dma_write(base, offset, value, type) \
    ({ \
        type vvs = value; \
        dma_memory_write(&address_space_memory, base + offset, &vvs, sizeof(type), MEMTXATTRS_UNSPECIFIED); \
    })

#endif
