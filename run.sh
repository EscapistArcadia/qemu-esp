#!/usr/bin/env bash

REBUILD_QEMU=0
CLEAN_REBUILD_QEMU=0
REBUILD_ESP=0
CLEAN_REBUILD_ESP=0
QEMU_STDIO=0
RUN_WITH_GDB=0

QEMU_ROOT="$PWD"
RISCV_TOOLCHAIN_PATH="$HOME/riscv"
ESP_ROOT="$HOME/esp_virtuoso"

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
        --dts)
            if [[ $# -gt 1 ]]; then
                if [[ -v DTB ]]; then
                    echo "Warning: --dtb option overrides --dts option. Using DTB file: $DTB"
                else
                    DTS=$(realpath "$2")
                fi
                if [[ ! -f "$DTS" ]]; then
                    echo "Error: DTS file does not exist: $DTS"
                    exit 1
                fi
            else
                echo "Error: --dts option requires an argument."
                exit 1
            fi
            shift 2
            ;;
        --dtb)
            if [[ $# -gt 1 ]]; then
                DTB=$(realpath "$2")
                if [[ -v DTS ]]; then
                    unset DTS
                    echo "Warning: --dtb option overrides --dts option. Using DTB file: $DTB"
                elif [[ ! -f "$DTB" ]]; then
                    echo "Error: DTB file does not exist: $DTB"
                    exit 1
                fi
            else
                echo "Error: --dtb option requires an argument."
                exit 1
            fi
            shift 2
            ;;
        --rebuild-qemu)
            REBUILD_QEMU=1
            shift
            ;;
        --rebuild-esp)
            REBUILD_ESP=1
            shift
            ;;
        --clean-rebuild-qemu)
            CLEAN_REBUILD_QEMU=1
            shift
            ;;
        --clean-rebuild-esp)
            CLEAN_REBUILD_ESP=1
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

if [[ ! -v DTB ]] && [[ ! -v DTS ]]; then
    echo "Error: Either --dtb or --dts option must be provided."
    DTB="$QEMU_ROOT/riscv.dtb"
fi

QEMU_BUILD="$QEMU_ROOT/build"
QEMU_EXECUTABLE="$QEMU_BUILD/qemu-system-riscv64"

ESP_SOC="$ESP_ROOT/socs/xilinx-vcu118-xcvu9p"
ESP_LINUX_ARIANE_ROOT="$ESP_ROOT/soft/ariane/linux"
ESP_LINUX_ARIANE_BUILD="$ESP_SOC/soft-build/ariane/linux-build"
ESP_LINUX_ARIANE_CONFIG="$ESP_LINUX_ARIANE_BUILD/.config"
ESP_LINUX_IMAGE="$ESP_LINUX_ARIANE_BUILD/arch/riscv/boot/Image"
ESP_OPENSBI_BUILD="$ESP_SOC/soft-build/ariane/opensbi-build"
ESP_OPENSBI_FIRMWARE="$ESP_OPENSBI_BUILD/platform/esp-fpga/firmware/fw_payload.elf"
ESP_FILESYS_IMAGE="$ESP_SOC/soft-build/ariane/sysroot.cpio"
ESP_DTB="$QEMU_ROOT/riscv.dtb"

export PATH="$HOME/riscv/bin:$PATH" # involve the built riscv toolchain

if [[ ! -d "$ESP_ROOT" ]]; then
    echo "Error: ESP_ROOT directory does not exist: $ESP_ROOT"
    exit 1
fi

if [[ $REBUILD_QEMU -eq 1 ]] || [[ $CLEAN_REBUILD_QEMU -eq 1 ]] || [[ ! -f "$QEMU_EXECUTABLE" ]]; then
    if [[ ! -d "$QEMU_BUILD" ]]; then
        mkdir "$QEMU_BUILD"
    fi
    pushd "$QEMU_BUILD"
    if [[ ! -f "config.status" ]]; then # check if QEMU has been configured
        ../configure --target-list=riscv64-softmmu --enable-debug
    fi
    if [[ $CLEAN_REBUILD_QEMU -eq 1 ]]; then
        if command -v ninja >/dev/null 2>&1; then
            ninja clean
        else
            make clean
        fi
    fi
    if command -v ninja >/dev/null 2>&1; then
        ninja qemu-system-riscv64
    else
        make -j "$(nproc)"
    fi
    popd
fi

REBUILD_LINUX=0
if [[ $REBUILD_ESP -eq 1 ]]  || [[ $CLEAN_REBUILD_ESP -eq 1 ]] || [[ ! -f "$ESP_LINUX_IMAGE" ]] || [[ ! -f "$ESP_FILESYS_IMAGE" ]] || [[ ! -f "$ESP_LINUX_ARIANE_CONFIG" ]]; then
    pushd "$ESP_SOC"

    export PATH="$RISCV_TOOLCHAIN_PATH/bin:$PATH"
    export RISCV="$RISCV_TOOLCHAIN_PATH"
    export ESP_ROOT="$ESP_ROOT"

    CONFIG_ARGS=(
        "$ESP_LINUX_ARIANE_ROOT/scripts/config"
        "--file" "$ESP_LINUX_ARIANE_CONFIG"
        "-e" "SERIAL_8250"
        "-e" "SERIAL_8250_CONSOLE"
        "-e" "SERIAL_OF_PLATFORM"
        "-e" "SERIAL_EARLYCON"
        "-e" "SERIAL_EARLYCON_RISCV_SBI"
        "-e" "HVC_DRIVER"
        "-e" "HVC_RISCV_SBI"
        "-e" "DEBUG_INFO"
    )

    if [[ $CLEAN_REBUILD_ESP -eq 1 ]]; then
        REBUILD_LINUX=1
        make linux-distclean
    elif [[ ! -f "$ESP_LINUX_ARIANE_CONFIG" ]]; then
        REBUILD_LINUX=1
    else
        pushd "$ESP_LINUX_ARIANE_ROOT"
        "${CONFIG_ARGS[@]}"
        popd
    fi

    make linux -j `nproc`

    if [[ $REBUILD_LINUX -eq 1 ]]; then
        pushd "$ESP_LINUX_ARIANE_ROOT"
        "${CONFIG_ARGS[@]}"
        popd
        make linux -j `nproc`
    fi
    popd
fi

if [[ -v DTS ]] && [[ ! -v DTB ]] && [[ -f "$DTS" ]]; then
    DTB=$(basename "$DTS" .dts).dtb
    dtc -I dts -O dtb "$DTS" > "$DTB"
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
    # "-bios" "$ESP_OPENSBI_FIRMWARE"
    "-kernel" "$ESP_LINUX_IMAGE"
    "-initrd" "$ESP_FILESYS_IMAGE"
    "-dtb" "$DTB"
    "-append" "\"console=ttyS0,115200\""
)

if [[ $QEMU_STDIO -eq 1 ]]; then
    QEMU_ARGS+=(
        "-display" "none"
        "-serial" "stdio"
    )
fi

echo "${QEMU_ARGS[@]}"
"${QEMU_ARGS[@]}"
