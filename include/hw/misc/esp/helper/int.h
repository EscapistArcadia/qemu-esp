#ifndef ESP_HELPER_INT_H
#define ESP_HELPER_INT_H

#define MAKEDWORD(lo, hi) (((uint64_t)(hi) << 32) | (uint64_t)(lo))

#define LOWORD(x) ((uint32_t)((x) & 0xffffffff))
#define HIWORD(x) ((uint32_t)(((x) >> 32) & 0xffffffff))

#endif
