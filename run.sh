#!/usr/bin/env bash

REBUILD_QEMU=0
REBUILD_ESP=0
QEMU_STDIO=0
RUN_WITH_GDB=0

QEMU_ROOT="$PWD"
# RISCV_TOOLCHAIN_PATH="$HOME/riscv"
# ESP_ROOT="$HOME/esp_virtuoso"

while [[ $# -gt 0 ]]; do
    key=$1

    case $key in
        --riscv)
            if [[ $# -gt 1 ]]; then
                RISCV_TOOLCHAIN_PATH=$(realpath "$2")
                shift 2
            else
                echo "Error: --riscv option requires an argument."
                exit 1
            fi
            ;;
        --esp)
            if [[ $# -gt 1 ]]; then
                ESP_ROOT=$(realpath "$2")
                shift 2
            else
                echo "Error: --esp option requires an argument."
                exit 1
            fi
            ;;
        --rebuild-qemu)
            REBUILD_QEMU=1
            shift
            ;;
        --rebuild-esp)
            REBUILD_ESP=1
            shift
            ;;
        --nographic)
            QEMU_STDIO=1
            shift
            ;;
        --gdb)
            RUN_WITH_GDB=1
            shift
            ;;
        *)
            echo "Unknown option: $key"
            exit 1
            ;;
    esac
done

QEMU_BUILD="$QEMU_ROOT/build"
QEMU_EXECUTABLE="$QEMU_BUILD/qemu-system-riscv64"

ESP_SOC="$ESP_ROOT/socs/xilinx-vcu118-xcvu9p"
ESP_LINUX_IMAGE="$ESP_SOC/soft-build/ariane/linux-build/arch/riscv/boot/Image"
ESP_FILESYS_IMAGE="$ESP_SOC/soft-build/ariane/sysroot.cpio"
ESP_DTB="$QEMU_ROOT/riscv.dtb"

export PATH="$HOME/riscv/bin:$PATH" # involve the built riscv toolchain

if [[ ! -d "$ESP_ROOT" ]]; then
    echo "Error: ESP_ROOT directory does not exist: $ESP_ROOT"
    exit 1
fi

if [[ $REBUILD_QEMU -eq 1 ]] || [[ ! -f "$QEMU_EXECUTABLE" ]]; then
    if [[ ! -d "$QEMU_BUILD" ]]; then
        mkdir "$QEMU_BUILD"
    fi
    cd "$QEMU_BUILD"
    if [[ ! -f "config.status" ]]; then # check if QEMU has been configured
        ../configure --target-list=riscv64-softmmu --enable-debug
    fi
    if command -v ninja >/dev/null 2>&1; then
        ninja qemu-system-riscv64
    else
        make -j "$(nproc)"
    fi
    cd "$QEMU_ROOT"
fi

if [[ $REBUILD_ESP -eq 1 ]] || [[ ! -f "$ESP_LINUX_IMAGE" ]] || [[ ! -f "$ESP_FILESYS_IMAGE" ]]; then
    cd "$ESP_SOC"
    export PATH="$RISCV_TOOLCHAIN_PATH/bin:$PATH"
    export RISCV="$RISCV_TOOLCHAIN_PATH"
    export ESP_ROOT="$ESP_ROOT"
    make linux -j `nproc`
fi

QEMU_ARGS=()

if [[ $RUN_WITH_GDB -eq 1 ]]; then
    QEMU_ARGS+=("gdb" "--args")
fi

QEMU_ARGS+=(
    "$QEMU_EXECUTABLE"
    "-machine" "virt"
    "-m" "512M"
    "-smp" "1"
    "-bios" "default"
    "-kernel" "$ESP_LINUX_IMAGE"
    "-initrd" "$ESP_FILESYS_IMAGE"
    "-dtb" "$ESP_DTB"
    "-append" "console=ttyS0,115200 rdinit=/init"
)

if [[ $QEMU_STDIO -eq 1 ]]; then
    QEMU_ARGS+=(
        "-display" "none"
        "-serial" "stdio"
    )
fi

"${QEMU_ARGS[@]}"
