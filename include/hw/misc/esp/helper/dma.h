#ifndef ESP_HELPER_DMA_H
#define ESP_HELPER_DMA_H

// #include "system/address-spaces.h"
// #include "system/dma.h"

#define dma_read(base, offset, type) \
    ({ \
        type value; \
        dma_memory_read(&address_space_memory, base + offset, &value, sizeof(type), MEMTXATTRS_UNSPECIFIED); \
        value; \
    })

#define dma_write(base, offset, value, type) \
    do { \
        dma_memory_write(&address_space_memory, base + offset, &value, sizeof(type), MEMTXATTRS_UNSPECIFIED); \
    } while (0)

#endif
