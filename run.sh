#!/usr/bin/env bash

set -e

REBUILD_QEMU=0
CLEAN_REBUILD_QEMU=0
REBUILD_LINUX=0
CLEAN_REBUILD_LINUX=0
REBUILD_FILESYSTEM=0
QEMU_NOGRAPHIC=0
RUN_WITH_GDB=0
DEBUG_LINUX=0
QEMU_GUEST_LINUX=0

QEMU_ROOT="$PWD"
RISCV_TOOLCHAIN_PATH="$HOME/riscv"
ESP_ROOT="$HOME/esp_virtuoso"
HOST_GDB_COMMANDS=()
GUEST_GDB_COMMANDS=()
GUEST_LINUX_COMMANDS=()
BOARD_SUFFIX=""
EXAMPLES=()

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
        --board-suffix)
            if [[ $# -gt 1 ]]; then
                BOARD_SUFFIX="$2"
                shift 2
            else
                echo "Error: --board-suffix option requires an argument."
                exit 1
            fi
            ;;
        --example)
            shift
            while [[ $# -gt 0 && ! "$1" == --* ]]; do
                EXAMPLES+=("$1")
                shift
            done
            ;;
        --cmd)
            # Add the following lines to the end of the /etc/init.d/rcS
            # AUTORUN=$(cat /proc/cmdline | tr ' ' '\n' | grep '^autorun=' | cut -d= -f2-)
            # [ -n "$AUTORUN" ] && eval "$AUTORUN"
            shift
            while [[ $# -gt 0 && ! "$1" == --* ]]; do
                GUEST_LINUX_COMMANDS+=("$1")
                shift
            done
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
        --rebuild-linux)
            REBUILD_LINUX=1
            shift
            ;;
        --rebuild-filesys)
            REBUILD_FILESYSTEM=1
            shift
            ;;
        --clean-rebuild-qemu)
            CLEAN_REBUILD_QEMU=1
            shift
            ;;
        --clean-rebuild-linux)
            CLEAN_REBUILD_LINUX=1
            shift
            ;;
        --nographic)
            QEMU_NOGRAPHIC=1
            shift
            ;;
        --gdb-qemu)
            RUN_WITH_GDB=1
            shift

            while [[ $# -gt 0 && ! "$1" == --* ]]; do
                HOST_GDB_COMMANDS+=("$1")
                shift
            done
            ;;
        --gdb-linux)
            QEMU_GUEST_LINUX=1
            shift

            while [[ $# -gt 0 && ! "$1" == --* ]]; do
                GUEST_GDB_COMMANDS+=("$1")
                shift
            done
            ;;
        --debug-linux)
            DEBUG_LINUX=1
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

BOARD_NAME="xilinx-vcu118-xcvu9p"
BOARD_DIR=$BOARD_NAME
if [[ -n "$BOARD_SUFFIX" ]]; then
    BOARD_DIR="${BOARD_NAME}-${BOARD_SUFFIX}"
fi
ESP_SOC="$ESP_ROOT/socs/${BOARD_DIR}"
ESP_LINUX_ARIANE_ROOT="$ESP_ROOT/soft/ariane/linux"
ESP_LINUX_ARIANE_BUILD="$ESP_SOC/soft-build/ariane/linux-build"
ESP_SYSROOT_ARIANE="$ESP_SOC/soft-build/ariane/sysroot"
ESP_LINUX_ARIANE_CONFIG="$ESP_LINUX_ARIANE_BUILD/.config"
ESP_LINUX_IMAGE="$ESP_LINUX_ARIANE_BUILD/arch/riscv/boot/Image"
ESP_LINUX_VMLINUX="$ESP_LINUX_ARIANE_BUILD/vmlinux"
ESP_OPENSBI_BUILD="$ESP_SOC/soft-build/ariane/opensbi-build"
ESP_OPENSBI_FIRMWARE="$ESP_OPENSBI_BUILD/platform/esp-fpga/firmware/fw_payload.elf"
ESP_FILESYS_LIST="$ESP_SOC/soft-build/ariane/sysroot.files"
ESP_FILESYS_IMAGE="$ESP_SOC/soft-build/ariane/sysroot.cpio"
VIRTUAL_ACC_APP_ROOT="$ESP_ROOT/soft/ariane/virtual-acc-app"
VIRTUAL_ACC_APP_MAKEFILE="$VIRTUAL_ACC_APP_ROOT/Makefile"
VIRTUAL_ACC_APP_EXAMPLES="$VIRTUAL_ACC_APP_ROOT/examples"
# ESP_DTB="$QEMU_ROOT/riscv.dtb"

# Runs RISCV GDB to debug Linux running on QEMU.
if [[ $QEMU_GUEST_LINUX -eq 1 ]]; then
    if [[ ! -f "$ESP_LINUX_IMAGE" ]]; then
        echo "Error: Linux image does not exist: $ESP_LINUX_IMAGE"
        exit 1
    fi

    GUEST_GDB_ARGS=(
        # "$RISCV_TOOLCHAIN_PATH/bin/riscv64-unknown-linux-gnu-gdb"
        "gdb-multiarch"
        "$ESP_LINUX_VMLINUX"
        "-ex" "set architecture riscv:rv64"
        "-ex" "target remote :1234"
        "-ex" "cd $ESP_LINUX_ARIANE_BUILD"
        "-ex" "source $ESP_LINUX_ARIANE_BUILD/vmlinux-gdb.py"
        # "-ex" "cd $QEMU_ROOT"
        # "-ex" "cd $ESP_SYSROOT_ARIANE/opt"
        # "-ex" "b arch_cpu_idle"
        # "-ex" "d 1"
        # "-ex" "c"
        "-ex" "echo Run the following commands to load symbols from modules AFTER the login prompt:\\n"
        "-ex" "echo \\tlx-symbols $ESP_SYSROOT_ARIANE/opt\\n"
    )

    for cmd in "${GUEST_GDB_COMMANDS[@]}"; do
        GUEST_GDB_ARGS+=(-ex "$cmd")
    done

    echo "${GUEST_GDB_ARGS[@]}"
    "${GUEST_GDB_ARGS[@]}"
    exit 0
fi

if [[ ! -d "$ESP_ROOT" ]]; then
    echo "Error: ESP_ROOT directory does not exist: $ESP_ROOT"
    exit 1
fi

if [[ ! -v DTB ]] && [[ ! -v DTS ]]; then
    echo "Error: Either --dtb or --dts option must be provided."
    DTB="$QEMU_ROOT/riscv.dtb"
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

export PATH="$RISCV_TOOLCHAIN_PATH/bin:$PATH"
export RISCV="$RISCV_TOOLCHAIN_PATH"
export ESP_ROOT="$ESP_ROOT"

if [[ ${#EXAMPLES[@]} -gt 0 ]]; then
    if [[ ! -f "$ESP_FILESYS_IMAGE" ]]; then
        pushd "$ESP_SOC"
        make linux -j `nproc`
        popd
    fi

    sed -i "67c ESP_EXE_DIR = $ESP_ROOT/socs/$BOARD_DIR/soft-build/ariane/sysroot/applications/test" "$VIRTUAL_ACC_APP_MAKEFILE"

    # Build examples under "virtual-acc-app"
    for example in "${EXAMPLES[@]}"; do
        for d in $(ls -d $VIRTUAL_ACC_APP_EXAMPLES/*/); do
            if [[ "$d" == *"$example"* ]]; then
                pushd "$d"
                make clean
                make -j `nproc` ENABLE_SM=1 ENABLE_VIRT=1
                popd
            fi
        done
    done

    REBUILD_FILESYSTEM=1
fi

if [[ $REBUILD_FILESYSTEM -eq 1 ]] || [[ ! -f "$ESP_FILESYS_IMAGE" ]]; then
    pushd "$ESP_SOC"
    
    rm "$ESP_FILESYS_LIST" "$ESP_FILESYS_IMAGE" || true
    make $ESP_LINUX_VMLINUX -j `nproc`

    popd
fi

BUILD_LINUX_TWICE=0
# Build Linux image and file system image if necessary
if [[ $REBUILD_LINUX -eq 1 ]]  || [[ $CLEAN_REBUILD_LINUX -eq 1 ]] || [[ ! -f "$ESP_LINUX_IMAGE" ]] || [[ ! -f "$ESP_FILESYS_IMAGE" ]] || [[ ! -f "$ESP_LINUX_ARIANE_CONFIG" ]]; then
    pushd "$ESP_SOC"

    CONFIG_ARGS=(
        "$ESP_LINUX_ARIANE_ROOT/scripts/config"
        "--file" "$ESP_LINUX_ARIANE_CONFIG"

        # Enable NS16550A UART driver for console output inside QEMU
        "-e" "SERIAL_8250"
        "-e" "SERIAL_8250_CONSOLE"
        "-e" "SERIAL_OF_PLATFORM"
        "-e" "SERIAL_EARLYCON"
        "-e" "SERIAL_EARLYCON_RISCV_SBI"
        "-e" "HVC_DRIVER"
        "-e" "HVC_RISCV_SBI"

        # Enable GDB scripts and debug info for better debugging experience
        "-e" "DEBUG_INFO"
        "-e" "GDB_SCRIPTS"
        "-e" "FRAME_POINTER"
        "-e" "KALLSYMS"
        "-e" "KALLSYMS_ALL"
    )

    if [[ $CLEAN_REBUILD_LINUX -eq 1 ]]; then
        BUILD_LINUX_TWICE=1
        make linux-distclean
    elif [[ ! -f "$ESP_LINUX_ARIANE_CONFIG" ]]; then
        BUILD_LINUX_TWICE=1
    else
        pushd "$ESP_LINUX_ARIANE_ROOT"
        "${CONFIG_ARGS[@]}"
        popd
    fi

    make linux -j `nproc`

    if [[ $BUILD_LINUX_TWICE -eq 1 ]]; then
        pushd "$ESP_LINUX_ARIANE_ROOT"
        "${CONFIG_ARGS[@]}"
        popd
        make linux -j `nproc`
    fi
    popd
fi

# Generate DTB from DTS if necessary
if [[ -v DTS ]] && [[ ! -v DTB ]] && [[ -f "$DTS" ]]; then
    DTB=$(basename "$DTS" .dts).dtb
    dtc -@ -I dts -O dtb "$DTS" > "$DTB"
fi

KERNEL_CMDLINE="console=ttyS0,115200"
if [[ ${#GUEST_LINUX_COMMANDS[@]} -gt 0 ]]; then
    AUTORUN_STR="${GUEST_LINUX_COMMANDS[0]}"
    for cmd in "${GUEST_LINUX_COMMANDS[@]:1}"; do
        AUTORUN_STR+="&&$cmd"
    done
    KERNEL_CMDLINE+=" autorun=$AUTORUN_STR"
fi

HOST_QEMU_ARGS=()

if [[ $RUN_WITH_GDB -eq 1 ]]; then
    HOST_QEMU_ARGS+=("gdb")

    for ex in "${HOST_GDB_COMMANDS[@]}"; do
        HOST_QEMU_ARGS+=(-ex "$ex")
    done

    HOST_QEMU_ARGS+=("--args")
fi

HOST_QEMU_ARGS+=(
    "$QEMU_EXECUTABLE"
    "-machine" "virt"
    "-m" "1G"
    "-smp" "1"
    "-bios" "default"
    # "-bios" "$ESP_OPENSBI_FIRMWARE"
    "-kernel" "$ESP_LINUX_IMAGE"
    # "-initrd" "$ESP_FILESYS_IMAGE"
    "-dtb" "$DTB"
    "-append" "$KERNEL_CMDLINE"
)

if [[ $QEMU_NOGRAPHIC -eq 1 ]]; then
    if [[ $RUN_WITH_GDB -eq 1 ]]; then
        HOST_QEMU_ARGS+=(
            "-display" "none"
            "-serial" "stdio"
        )
    else
        HOST_QEMU_ARGS+=(
            "-nographic"
        )
    fi
fi

if [[ $DEBUG_LINUX -eq 1 ]]; then
    HOST_QEMU_ARGS+=(
        "-S" # pause CPU at reset, wait for GDB connection
        "-s" # shorthand for -gdb tcp::1234
    )
fi

echo "${HOST_QEMU_ARGS[@]}"

export DEBUGINFOD_URLS=
"${HOST_QEMU_ARGS[@]}"
