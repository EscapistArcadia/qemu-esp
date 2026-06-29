#include "hw/misc/esp/accelerator.h"

#include "hw/misc/esp/regmap.h"
#include "hw/misc/esp/helper/int.h"
#include "hw/misc/esp/helper/dma.h"
#include "hw/misc/esp/accelerators/template.h"

#include "hw/misc/esp/sm_queue.h"

#define ESP_ACCELERATOR_MMIO_NAME "esp-accelerator-mmio"
#define ESP_ACCELERATOR_MMIO_BASE (0x60010000)
#define ESP_ACCELERATOR_MMIO_SIZE (0x100)

#if 0
#include "accelerator_mmio_verbose.inc"
#else
#define esp_accelerator_mmio_read_verbose(addr, size)
#define esp_accelerator_mmio_write_verbose(addr, content, size)
#endif

#define IDLE_PRIORITY 5

/*
 * Round-robin scheduler: starting from current_context_next, pick the first
 * valid non-idle context, then override it if a later candidate has strictly
 * lower nprio.  Sets current_context_next to one past the first candidate
 * found, preserving the rotation across calls.
 */
void pick_context_rr(ESPAcceleratorState *s, bool bias_switch) {
    uint32_t new_context = s->current_context;
    uint32_t min_nprio   = UINT32_MAX;
    bool     rr_found    = false;
    uint32_t idx         = s->current_context_next;

    for (int i = 0; i < MAX_CONTEXTS; i++, idx++) {
        uint32_t candidate = idx % MAX_CONTEXTS;

        if (!(s->prev_valid_contexts & (1u << candidate))) {
            s->context_vruntime[candidate] = s->sched_period > 0 ? s->sched_period - 1 : 0;
            s->context_runtime[candidate]  = 0;
            continue;
        }
        uint32_t prio = s->nprio[candidate];
        if (prio >= IDLE_PRIORITY)
            continue;
        if (bias_switch && candidate == s->current_context)
            continue;

        if (!rr_found) {
            new_context             = candidate;
            s->current_context_next = candidate + 1;
            min_nprio               = prio;
            rr_found                = true;
        } else if (prio < min_nprio) {
            new_context = candidate;
            min_nprio   = prio;
        }
    }

    s->current_context = new_context;
}

/*
 * Fair scheduler: pick the context with the lowest vruntime; break ties by
 * choosing the one with the lowest nprio.  Sets current_context_next to one
 * past the winner to keep a secondary round-robin tiebreak across calls.
 */
void pick_context_fair(ESPAcceleratorState *s, bool bias_switch) {
    uint32_t new_context  = s->current_context;
    uint32_t min_vruntime = UINT32_MAX;
    uint32_t min_nprio    = UINT32_MAX;
    uint32_t idx          = s->current_context_next;

    for (int i = 0; i < MAX_CONTEXTS; i++, idx++) {
        uint32_t candidate = idx % MAX_CONTEXTS;

        if (!(s->prev_valid_contexts & (1u << candidate))) {
            s->context_vruntime[candidate] = s->sched_period > 0 ? s->sched_period - 1 : 0;
            s->context_runtime[candidate]  = 0;
            continue;
        }
        uint32_t prio = s->nprio[candidate];
        if (prio >= IDLE_PRIORITY)
            continue;
        if (bias_switch && candidate == s->current_context)
            continue;

        uint32_t vr = s->context_vruntime[candidate];
        if (vr < min_vruntime || (vr == min_vruntime && prio < min_nprio)) {
            min_vruntime = vr;
            min_nprio    = prio;
            new_context  = candidate;
        }
    }

    s->current_context_next = new_context + 1;
    s->current_context = new_context;
}

/*
 * Mirrors check_next_context() from esp_accelerator_amu.i.hpp.
 *
 * The valid_context_monitor thread's new/old context detection is inlined
 * here by diffing valid_contexts against prev_valid_contexts, since QEMU
 * has no separate hardware thread for that role.
 *
 * Cycle counting is omitted: vruntime is incremented by nprio per call
 * (one call ≈ one completed task dispatch), which preserves the fairness
 * property without needing a real cycle counter.
 *
 * Define SCHED_RR at compile time to select round-robin; the default is
 * SCHED_FAIR (lowest-vruntime, mirroring the HLS #else branch).
 */
static void check_next_context(ESPAcceleratorState *s, bool bias_switch)
{
    uint64_t sm_base = s->esp->sm.addr;
    uint32_t vld_ctxt;
    uint32_t min_vruntime;
    uint64_t q;
    int i;

    /* Spin until at least one valid context is present. */
    do {
        vld_ctxt = s->valid_contexts;
    } while (vld_ctxt == 0);

    /* Handle removed contexts: set their queues back to AVAIL. */
    uint32_t removed = s->prev_valid_contexts & ~vld_ctxt;
    for (i = 0; i < MAX_CONTEXTS; i++) {
        if (!(removed & (1u << i)))
            continue;
        if (s->queue_ptr[i] != 0) {
            q = sm_base + (uint64_t)s->queue_ptr[i] * sizeof(uint32_t);
            do {
                dma_write(q, offsetof(sm_queue_t, stat), (uint64_t)QUEUE_AVAIL, uint64_t);
                q = dma_read(q, offsetof(sm_queue_t, next_queue_ptr), uint64_t);
                if (q != 0) q = sm_base + q * sizeof(uint32_t); /* TODO: find a more elegant way to handle this */
            } while (q != 0);
        }
        s->valid_queue_ptr     &= ~(1u << i);
        s->context_runtime[i]  = 0;
        s->context_vruntime[i] = 0;
    }

    /* Compute min vruntime across active contexts (used to initialize new ones). */
    min_vruntime = UINT32_MAX;
    for (i = 0; i < MAX_CONTEXTS; i++) {
        if ((vld_ctxt & (1u << i)) && (s->valid_queue_ptr & (1u << i)) &&
            s->context_vruntime[i] < min_vruntime) {
            min_vruntime = s->context_vruntime[i];
        }
    }
    if (min_vruntime == UINT32_MAX)
        min_vruntime = 0;

    /* Handle new contexts: check queue availability, mark BUSY, set vruntime. */
    uint32_t added = vld_ctxt & ~s->prev_valid_contexts;
    for (i = 0; i < MAX_CONTEXTS; i++) {
        if (!(added & (1u << i)) || s->queue_ptr[i] == 0)
            continue;
        q = sm_base + (uint64_t)s->queue_ptr[i] * sizeof(uint32_t);
        bool queue_ready = true;
        do {
            uint64_t stat = dma_read(q, offsetof(sm_queue_t, stat), uint64_t);
            if (stat != QUEUE_AVAIL) {
                queue_ready = false;
                break;
            }
            q = dma_read(q, offsetof(sm_queue_t, next_queue_ptr), uint64_t);
            if (q != 0) q = sm_base + q * sizeof(uint32_t); /* TODO: find a more elegant way to handle this */
        } while (q != 0);

        if (queue_ready) {
            q = sm_base + (uint64_t)s->queue_ptr[i] * sizeof(uint32_t);
            do {
                dma_write(q, offsetof(sm_queue_t, stat), (uint64_t)QUEUE_BUSY, uint64_t);
                q = dma_read(q, offsetof(sm_queue_t, next_queue_ptr), uint64_t);
                if (q != 0) q = sm_base + q * sizeof(uint32_t); /* TODO: find a more elegant way to handle this */
            } while (q != 0);
            s->context_vruntime[i] = min_vruntime;
            s->valid_queue_ptr    |= (1u << i);
        }
    }

    s->prev_valid_contexts = vld_ctxt;

    /* Update vruntime of the current context (nprio units per dispatch). */
    uint32_t cur       = s->current_context;
    uint32_t nprio_cur = s->nprio[cur];
    if (s->sched_period > 0 &&
        s->context_vruntime[cur] + nprio_cur >= s->sched_period) {
        s->context_vruntime[cur] = s->sched_period;
    } else {
        s->context_vruntime[cur] += nprio_cur;
    }
    s->context_runtime[cur]++;

    /*
     * If all active non-idle contexts have reached the scheduling period,
     * reset vruntimes: valid contexts go to 0, invalid to sched_period-1.
     */
    if (s->sched_period > 0) {
        bool period_expired = true;
        for (i = 0; i < MAX_CONTEXTS; i++) {
            if ((vld_ctxt & (1u << i)) && s->nprio[i] < IDLE_PRIORITY &&
                s->context_vruntime[i] < s->sched_period) {
                period_expired = false;
                break;
            }
        }
        if (period_expired) {
            for (i = 0; i < MAX_CONTEXTS; i++) {
                s->context_vruntime[i] = (vld_ctxt & (1u << i))
                    ? 0 : s->sched_period - 1;
            }
        }
    }

    s->accel->pick_context(s, bias_switch);
}

static void *esp_accelerator_execute(void *opaque) {
    ESPAcceleratorState *s = opaque;

    uint64_t sm_base = s->esp->sm.addr;
    uint64_t input_queue_base;
    bool bias_switch = false;

    void *descriptor;
    const uint64_t descriptor_size = s->accel->conf_size + MPMC_DEGREE * sizeof(sm_queue_entry_t);

    descriptor = g_malloc(descriptor_size);

    while (1) {
        check_next_context(s, bias_switch);
        bias_switch = false;

        input_queue_base = sm_base + (uint64_t)s->queue_ptr[s->current_context] * sizeof(uint32_t);
        if (!sm_queue_can_pop(s, input_queue_base, descriptor, descriptor_size)) {
            bias_switch = true;
            continue;
        }

        if (!sm_queue_can_push(s, s->sm_info_output_queue[0])) {
            continue;
        }

        s->accel->execute(s, descriptor);

        sm_queue_pop(s, input_queue_base);
        sm_queue_push(s, s->sm_info_output_queue[0], s->sm_info_output_entry[0]);
    }

    return NULL;
}

static uint64_t esp_accelerator_mmio_read(void *opaque, hwaddr addr, unsigned size) {
    ESPAcceleratorState *s = opaque;

    if (addr > ESP_ACCELERATOR_MMIO_MAX) {
        return s->accel->mmio_read ? s->accel->mmio_read(s, addr, size) : 0;
    }

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

    if (addr > ESP_ACCELERATOR_MMIO_MAX) {
        if (s->accel->mmio_write) {
            s->accel->mmio_write(s, addr, data, size);
        }
        return;
    }

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

DeviceState *esp_accelerator_create(ESPSubsystemState *esp, ESPAccelerator *accel, hwaddr mmio_base, uint64_t mmio_size) {
    DeviceState *dev = qdev_new(TYPE_ESP_ACCELERATOR);
    ESPAcceleratorState *s = ESP_ACCELERATOR(dev);

    QLIST_INSERT_HEAD(&esp->accelerators, s, node);

    s->esp = esp;
    s->accel = accel;

    /* TODO: generate unique name */
    INIT_MMIO_REGION(s->mmio, "esp_accelerator", mmio_base, mmio_size);

    /* TODO: add return value check */
    qemu_thread_create(&s->execution, accel->type, esp_accelerator_execute, s, QEMU_THREAD_DETACHED);

    /* TODO: create other threads: cycle counter, valid context monitor, etc. */

    return dev;
}

static const TypeInfo esp_accelerator_info = {
    .name = TYPE_ESP_ACCELERATOR,
    /**
     * You know, I even found that this has been wrong for a long time just after I realized that this is actually correct. There should be no inheritance relationship between the accelerator and the subsystem. Otherwise, every time the accelerator is created, the subsystem will be created again, which is not what I desired.
     */
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(ESPAcceleratorState),

    // .class_init = esp_accelerator_class_init,
};

static void esp_accelerator_register_types(void) {
    type_register_static(&esp_accelerator_info);
}

type_init(esp_accelerator_register_types)
