static void gemm_stratus_mmio_read_verbose(hwaddr addr, unsigned size) {
    switch (addr) {
        case CMD_REG:
            printf("[QEMU] gemm_stratus: read CMD_REG\n");
            break;
        case STATUS_REG:
            printf("[QEMU] gemm_stratus: read STATUS_REG\n");
            break;
        case SELECT_REG:
            printf("[QEMU] gemm_stratus: read SELECT_REG\n");
            break;
        case DEVID_REG:
            printf("[QEMU] gemm_stratus: read DEVID_REG\n");
            break;
        case PT_ADDRESS_REG:
            printf("[QEMU] gemm_stratus: read PT_ADDRESS_REG\n");
            break;
        case PT_NCHUNK_REG:
            printf("[QEMU] gemm_stratus: read PT_NCHUNK_REG\n");
            break;
        case PT_SHIFT_REG:
            printf("[QEMU] gemm_stratus: read PT_SHIFT_REG\n");
            break;
        case PT_NCHUNK_MAX_REG:
            printf("[QEMU] gemm_stratus: read PT_NCHUNK_MAX_REG\n");
            break;
        case PT_ADDRESS_EXTENDED_REG:
            printf("[QEMU] gemm_stratus: read PT_ADDRESS_EXTENDED_REG\n");
            break;
        case PT_COHERENCE_REG:
            printf("[QEMU] gemm_stratus: read PT_COHERENCE_REG\n");
            break;
        case P2P_REG:
            printf("[QEMU] gemm_stratus: read P2P_REG\n");
            break;
        case YX_REG:
            printf("[QEMU] gemm_stratus: read YX_REG\n");
            break;
        case SRC_OFFSET_REG:
            printf("[QEMU] gemm_stratus: read SRC_OFFSET_REG\n");
            break;
        case DST_OFFSET_REG:
            printf("[QEMU] gemm_stratus: read DST_OFFSET_REG\n");
            break;
        case SPANDEX_REG:
            printf("[QEMU] gemm_stratus: read SPANDEX_REG\n");
            break;
        case VALID_CONTEXTS_ACK_REG:
            printf("[QEMU] gemm_stratus: read VALID_CONTEXTS_ACK_REG\n");
            break;
        case PT_ADDRESS_REG_0:
            printf("[QEMU] gemm_stratus: read PT_ADDRESS_REG_0\n");
            break;
        case PT_ADDRESS_REG_1:
            printf("[QEMU] gemm_stratus: read PT_ADDRESS_REG_1\n");
            break;
        case PT_ADDRESS_REG_2:
            printf("[QEMU] gemm_stratus: read PT_ADDRESS_REG_2\n");
            break;
        case PT_ADDRESS_REG_3:
            printf("[QEMU] gemm_stratus: read PT_ADDRESS_REG_3\n");
            break;
        case MON_UTIL_REG_0_LO:
            printf("[QEMU] gemm_stratus: read MON_UTIL_REG_0_LO\n");
            break;
        case MON_UTIL_REG_0_HI:
            printf("[QEMU] gemm_stratus: read MON_UTIL_REG_0_HI\n");
            break;
        case MON_UTIL_REG_1_LO:
            printf("[QEMU] gemm_stratus: read MON_UTIL_REG_1_LO\n");
            break;
        case MON_UTIL_REG_1_HI:
            printf("[QEMU] gemm_stratus: read MON_UTIL_REG_1_HI\n");
            break;
        case MON_UTIL_REG_2_LO:
            printf("[QEMU] gemm_stratus: read MON_UTIL_REG_2_LO\n");
            break;
        case MON_UTIL_REG_2_HI:
            printf("[QEMU] gemm_stratus: read MON_UTIL_REG_2_HI\n");
            break;
        case MON_UTIL_REG_3_LO:
            printf("[QEMU] gemm_stratus: read MON_UTIL_REG_3_LO\n");
            break;
        case MON_UTIL_REG_3_HI:
            printf("[QEMU] gemm_stratus: read MON_UTIL_REG_3_HI\n");
            break;
        case AMU_INFO_QUEUE_PTR_REG_0:
            printf("[QEMU] gemm_stratus: read AMU_INFO_QUEUE_PTR_REG_0\n");
            break;
        case AMU_INFO_QUEUE_PTR_REG_1:
            printf("[QEMU] gemm_stratus: read AMU_INFO_QUEUE_PTR_REG_1\n");
            break;
        case AMU_INFO_QUEUE_PTR_REG_2:
            printf("[QEMU] gemm_stratus: read AMU_INFO_QUEUE_PTR_REG_2\n");
            break;
        case AMU_INFO_QUEUE_PTR_REG_3:
            printf("[QEMU] gemm_stratus: read AMU_INFO_QUEUE_PTR_REG_3\n");
            break;
        case AMU_INFO_NPRIO_REG_0:
            printf("[QEMU] gemm_stratus: read AMU_INFO_NPRIO_REG_0\n");
            break;
        case AMU_INFO_NPRIO_REG_1:
            printf("[QEMU] gemm_stratus: read AMU_INFO_NPRIO_REG_1\n");
            break;
        case AMU_INFO_NPRIO_REG_2:
            printf("[QEMU] gemm_stratus: read AMU_INFO_NPRIO_REG_2\n");
            break;
        case AMU_INFO_NPRIO_REG_3:
            printf("[QEMU] gemm_stratus: read AMU_INFO_NPRIO_REG_3\n");
            break;
        case AMU_INFO_VLD_CTXT_REG:
            printf("[QEMU] gemm_stratus: read AMU_INFO_VLD_CTXT_REG\n");
            break;
        case AMU_INFO_SCHED_PERIOD_REG:
            printf("[QEMU] gemm_stratus: read AMU_INFO_SCHED_PERIOD_REG\n");
            break;
        case GEMM_NINPUTS_REG:
            printf("[QEMU] gemm_stratus: read GEMM_NINPUTS_REG\n");
            break;
        case GEMM_D1_REG:
            printf("[QEMU] gemm_stratus: read GEMM_D1_REG\n");
            break;
        case GEMM_D2_REG:
            printf("[QEMU] gemm_stratus: read GEMM_D2_REG\n");
            break;
        case GEMM_D3_REG:
            printf("[QEMU] gemm_stratus: read GEMM_D3_REG\n");
            break;
        case GEMM_LD_OFFSET1_REG:
            printf("[QEMU] gemm_stratus: read GEMM_LD_OFFSET1_REG\n");
            break;
        case GEMM_LD_OFFSET2_REG:
            printf("[QEMU] gemm_stratus: read GEMM_LD_OFFSET2_REG\n");
            break;
        case GEMM_BIAS_OFFSET_REG:
            printf("[QEMU] gemm_stratus: read GEMM_BIAS_OFFSET_REG\n");
            break;
        case GEMM_ST_OFFSET_REG:
            printf("[QEMU] gemm_stratus: read GEMM_ST_OFFSET_REG\n");
            break;
        case GEMM_DO_RELU_REG:
            printf("[QEMU] gemm_stratus: read GEMM_DO_RELU_REG\n");
            break;
        case GEMM_DO_BIAS_REG:
            printf("[QEMU] gemm_stratus: read GEMM_DO_BIAS_REG\n");
            break;
        case GEMM_TRANSPOSE_REG:
            printf("[QEMU] gemm_stratus: read GEMM_TRANSPOSE_REG\n");
            break;        
        default:
            printf("[QEMU] gemm_stratus: read unknown address 0x%" HWADDR_PRIx "\n", addr);
            break;
    }
}

static void gemm_stratus_mmio_write_verbose(hwaddr addr, uint64_t data, unsigned size) {
    switch (addr) {
        case CMD_REG:
            printf("[QEMU] gemm_stratus: write CMD_REG, data=0x%" PRIx64 "\n", data);
            break;
        case STATUS_REG:
            printf("[QEMU] gemm_stratus: write STATUS_REG, data=0x%" PRIx64 "\n", data);
            break;
        case SELECT_REG:
            printf("[QEMU] gemm_stratus: write SELECT_REG, data=0x%" PRIx64 "\n", data);
            break;
        case DEVID_REG:
            printf("[QEMU] gemm_stratus: write DEVID_REG, data=0x%" PRIx64 "\n", data);
            break;
        case PT_ADDRESS_REG:
            printf("[QEMU] gemm_stratus: write PT_ADDRESS_REG, data=0x%" PRIx64 "\n", data);
            break;
        case PT_NCHUNK_REG:
            printf("[QEMU] gemm_stratus: write PT_NCHUNK_REG, data=0x%" PRIx64 "\n", data);
            break;
        case PT_SHIFT_REG:
            printf("[QEMU] gemm_stratus: write PT_SHIFT_REG, data=0x%" PRIx64 "\n", data);
            break;
        case PT_NCHUNK_MAX_REG:
            printf("[QEMU] gemm_stratus: write PT_NCHUNK_MAX_REG, data=0x%" PRIx64 "\n", data);
            break;
        case PT_ADDRESS_EXTENDED_REG:
            printf("[QEMU] gemm_stratus: write PT_ADDRESS_EXTENDED_REG, data=0x%" PRIx64 "\n", data);
            break;
        case PT_COHERENCE_REG:
            printf("[QEMU] gemm_stratus: write PT_COHERENCE_REG, data=0x%" PRIx64 "\n", data);
            break;
        case P2P_REG:
            printf("[QEMU] gemm_stratus: write P2P_REG, data=0x%" PRIx64 "\n", data);
            break;
        case YX_REG:
            printf("[QEMU] gemm_stratus: write YX_REG, data=0x%" PRIx64 "\n", data);
            break;
        case SRC_OFFSET_REG:
            printf("[QEMU] gemm_stratus: write SRC_OFFSET_REG, data=0x%" PRIx64 "\n", data);
            break;
        case DST_OFFSET_REG:
            printf("[QEMU] gemm_stratus: write DST_OFFSET_REG, data=0x%" PRIx64 "\n", data);
            break;
        case SPANDEX_REG:
            printf("[QEMU] gemm_stratus: write SPANDEX_REG, data=0x%" PRIx64 "\n", data);
            break;
        case VALID_CONTEXTS_ACK_REG:
            printf("[QEMU] gemm_stratus: write VALID_CONTEXTS_ACK_REG, data=0x%" PRIx64 "\n", data);
            break;
        case PT_ADDRESS_REG_0:
            printf("[QEMU] gemm_stratus: write PT_ADDRESS_REG_0, data=0x%" PRIx64 "\n", data);
            break;
        case PT_ADDRESS_REG_1:
            printf("[QEMU] gemm_stratus: write PT_ADDRESS_REG_1, data=0x%" PRIx64 "\n", data);
            break;
        case PT_ADDRESS_REG_2:
            printf("[QEMU] gemm_stratus: write PT_ADDRESS_REG_2, data=0x%" PRIx64 "\n", data);
            break;
        case PT_ADDRESS_REG_3:
            printf("[QEMU] gemm_stratus: write PT_ADDRESS_REG_3, data=0x%" PRIx64 "\n", data);
            break;
        case MON_UTIL_REG_0_LO:
            printf("[QEMU] gemm_stratus: write MON_UTIL_REG_0_LO, data=0x%" PRIx64 "\n", data);
            break;
        case MON_UTIL_REG_0_HI:
            printf("[QEMU] gemm_stratus: write MON_UTIL_REG_0_HI, data=0x%" PRIx64 "\n", data);
            break;
        case MON_UTIL_REG_1_LO:
            printf("[QEMU] gemm_stratus: write MON_UTIL_REG_1_LO, data=0x%" PRIx64 "\n", data);
            break;
        case MON_UTIL_REG_1_HI:
            printf("[QEMU] gemm_stratus: write MON_UTIL_REG_1_HI, data=0x%" PRIx64 "\n", data);
            break;
        case MON_UTIL_REG_2_LO:
            printf("[QEMU] gemm_stratus: write MON_UTIL_REG_2_LO, data=0x%" PRIx64 "\n", data);
            break;
        case MON_UTIL_REG_2_HI:
            printf("[QEMU] gemm_stratus: write MON_UTIL_REG_2_HI, data=0x%" PRIx64 "\n", data);
            break;
        case MON_UTIL_REG_3_LO:
            printf("[QEMU] gemm_stratus: write MON_UTIL_REG_3_LO, data=0x%" PRIx64 "\n", data);
            break;
        case MON_UTIL_REG_3_HI:
            printf("[QEMU] gemm_stratus: write MON_UTIL_REG_3_HI, data=0x%" PRIx64 "\n", data);
            break;
        case AMU_INFO_QUEUE_PTR_REG_0:
            printf("[QEMU] gemm_stratus: write AMU_INFO_QUEUE_PTR_REG_0, data=0x%" PRIx64 "\n", data);
            break;
        case AMU_INFO_QUEUE_PTR_REG_1:
            printf("[QEMU] gemm_stratus: write AMU_INFO_QUEUE_PTR_REG_1, data=0x%" PRIx64 "\n", data);
            break;
        case AMU_INFO_QUEUE_PTR_REG_2:
            printf("[QEMU] gemm_stratus: write AMU_INFO_QUEUE_PTR_REG_2, data=0x%" PRIx64 "\n", data);
            break;
        case AMU_INFO_QUEUE_PTR_REG_3:
            printf("[QEMU] gemm_stratus: write AMU_INFO_QUEUE_PTR_REG_3, data=0x%" PRIx64 "\n", data);
            break;
        case AMU_INFO_NPRIO_REG_0:
            printf("[QEMU] gemm_stratus: write AMU_INFO_NPRIO_REG_0, data=0x%" PRIx64 "\n", data);
            break;
        case AMU_INFO_NPRIO_REG_1:
            printf("[QEMU] gemm_stratus: write AMU_INFO_NPRIO_REG_1, data=0x%" PRIx64 "\n", data);
            break;
        case AMU_INFO_NPRIO_REG_2:
            printf("[QEMU] gemm_stratus: write AMU_INFO_NPRIO_REG_2, data=0x%" PRIx64 "\n", data);
            break;
        case AMU_INFO_NPRIO_REG_3:
            printf("[QEMU] gemm_stratus: write AMU_INFO_NPRIO_REG_3, data=0x%" PRIx64 "\n", data);
            break;
        case AMU_INFO_VLD_CTXT_REG:
            printf("[QEMU] gemm_stratus: write AMU_INFO_VLD_CTXT_REG, data=0x%" PRIx64 "\n", data);
            break;
        case AMU_INFO_SCHED_PERIOD_REG:
            printf("[QEMU] gemm_stratus: write AMU_INFO_SCHED_PERIOD_REG, data=0x%" PRIx64 "\n", data);
            break;
        case GEMM_NINPUTS_REG:
            printf("[QEMU] gemm_stratus: write GEMM_NINPUTS_REG, data=0x%" PRIx64 "\n", data);
            break;
        case GEMM_D1_REG:
            printf("[QEMU] gemm_stratus: write GEMM_D1_REG, data=0x%" PRIx64 "\n", data);
            break;
        case GEMM_D2_REG:
            printf("[QEMU] gemm_stratus: write GEMM_D2_REG, data=0x%" PRIx64 "\n", data);
            break;
        case GEMM_D3_REG:
            printf("[QEMU] gemm_stratus: write GEMM_D3_REG, data=0x%" PRIx64 "\n", data);
            break;
        case GEMM_LD_OFFSET1_REG:
            printf("[QEMU] gemm_stratus: write GEMM_LD_OFFSET1_REG, data=0x%" PRIx64 "\n", data);
            break;
        case GEMM_LD_OFFSET2_REG:
            printf("[QEMU] gemm_stratus: write GEMM_LD_OFFSET2_REG, data=0x%" PRIx64 "\n", data);
            break;
        case GEMM_BIAS_OFFSET_REG:
            printf("[QEMU] gemm_stratus: write GEMM_BIAS_OFFSET_REG, data=0x%" PRIx64 "\n", data);
            break;
        case GEMM_ST_OFFSET_REG:
            printf("[QEMU] gemm_stratus: write GEMM_ST_OFFSET_REG, data=0x%" PRIx64 "\n", data);
            break;
        case GEMM_DO_RELU_REG:
            printf("[QEMU] gemm_stratus: write GEMM_DO_RELU_REG, data=0x%" PRIx64 "\n", data);
            break;
        case GEMM_DO_BIAS_REG:
            printf("[QEMU] gemm_stratus: write GEMM_DO_BIAS_REG, data=0x%" PRIx64 "\n", data);
            break;
        case GEMM_TRANSPOSE_REG:
            printf("[QEMU] gemm_stratus: write GEMM_TRANSPOSE_REG, data=0x%" PRIx64 "\n", data);
            break;
        default:
            printf("[QEMU] gemm_stratus: write unknown address 0x%" HWADDR_PRIx "\n", addr);
            break;
    }
}
