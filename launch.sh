#!/bin/bash
set -e

### --- CONFIG --- ###

## ROOT Path

READ_REL_PATH=$(dirname "$0")
ROOT_PATH=$(realpath "${READ_REL_PATH}")
cd $ROOT_PATH

if [ -f secret.sh ]; then
    source secret.sh
fi

TODAY=$(date +%F_%H-%M-%S)
BENCHMARK_OUT="/opt/benchmark-out"
export BREAKDOWN=${BREAKDOWN:=}
BREAKDOWN_KRPS=1450

ENABLE_SILO="Y"
ENABLE_FAISS="Y"

## Compute Node Configuration ##

#! IMPORTANT: NEVER USE SPACES

BUILD_TYPE=RelWithDebInfo
# BUILD_TYPE=Release
# BUILD_TYPE=Debug
NO_SG=${NO_SG:=n}

# Network Configuration

ETH_IF=eno2
IP=172.16.0.2
SUBNET=255.255.255.0
PREFIX=24
GW=172.16.0.1
NAMESERVER=8.8.8.8
RDMA_IF=ens4f0np0     # to memory server
RDMA_IF_RAW=ens4f1np1 # to loadgen

RDMA_IP=192.168.123.2
RDMA_IP_RAW=192.168.124.2

# CPU Configuration

HYPERVISOR=qemu_microvm
NODE=0
CPU_START=1
CPU_LAST=14
TOTAL_CPUS=$((CPU_LAST - CPU_START + 1))

DISPATCHER_CPU=${DISPATCHER_CPU:=1}
WORKER_CPUS=${WORKER_CPUS:=8}
WORKER_CPU_START=${WORKER_CPU_START:=2}

MEMORY=${MEMORY:=10G}
PCI=device
PREFETCHER=${PREFETCHER:=}
TRACE=
# TRACE="pusnow_yield*,pusnow_async*,pusnow_exit*,pusnow_fetch*"
AUTO_POWEROFF=${AUTO_POWEROFF:=y}

# Remote Configuration

DDC_DISABLE=${DDC_DISABLE:=}

# Infiniband Configuration

IB_DEVICE=mlx5_0
IB_UVERBS=uverbs0
ETH_DEVICE=mlx5_1
ETH_UVERBS=uverbs1
# IB_PORT=1
# GID_IDX=1

## Compute Node Configuration (END) ##

## Memory Node Configuration ##
MS_ETH_IF=eno2
MS_SSH=shader-gosan.anlab
MS_IP=172.16.0.3
MS_PORT=12345
MS_DEVNAME=mlx5_0
MS_NODE=0
MS_RDMA_IF=ens4f0np0

MS_RDMA_IP=192.168.123.3
MS_MEMORY_GB=128
MS_HUGE_TLBS=200G
## Memory Node Configuration (END) ##

## Load Generator Configuration ##
LG_SSH=shader-jeju.anlab
LG_DEVNAME=mlx5_0
LG_NODE=0
LG_RDMA_IF=ens7f0np0
LG_RDMA_IP=192.168.124.3
LG_PORT=1
LG_RX_CQ=2048
LG_RX_WR=2048
LG_OUTPUT=${LG_OUTPUT:=/tmp}

## dynamic

LG_CORE_START=${LG_CORE_START:=1}
LG_RPS=${LG_RPS:=10000}

LG_NUM_OPERATION=${LG_NUM_OPERATION:=0}
LG_DURATION=${LG_DURATION:=10}

LG_COUNT_DIST=${LG_COUNT_DIST:=static}

SCAN_KEYSPACE_MEMCACHED=${SCAN_KEYSPACE_MEMCACHED:=36200000}
SCAN_KEYSPACE_MEMCACHED2=${SCAN_KEYSPACE_MEMCACHED2:=178000000} # 178000000 for 128

SCAN_KEYLEN=${SCAN_KEYLEN:=50}
SCAN_VALLEN=${SCAN_VALLEN:=1024}
SCAN_VALLEN2=${SCAN_VALLEN2:=128}

## exp

WORKLOAD=${WORKLOAD:="ping"}
IS_NUM=${IS_NUM:="1"}

APP_MEMSIZE=40
# NUM_WAREHOUSES=576
NUM_WAREHOUSES=32

declare -a MEMORY_SIZES=("72G" "36G" "18G" "10G" "6G")
# declare -a MEMORY_SIZES=("36G")

declare -A WORKLOAD_ARGS_LIST
WORKLOAD_ARGS_LIST["echo"]=""
WORKLOAD_ARGS_LIST["array-1"]="--app_array64_enable 1 --app_memsize ${APP_MEMSIZE}"
WORKLOAD_ARGS_LIST["warray-1"]="--app_array64_enable 1 --app_memsize ${APP_MEMSIZE}"
WORKLOAD_ARGS_LIST["array-100"]="--app_array64_enable 1 --app_memsize ${APP_MEMSIZE}"
WORKLOAD_ARGS_LIST["array-bm"]="--app_array64_enable 1 --app_memsize ${APP_MEMSIZE}"
WORKLOAD_ARGS_LIST["scan-1"]="--app_rocksdb_enable 1"
WORKLOAD_ARGS_LIST["scan-100"]="--app_rocksdb_enable 1"
WORKLOAD_ARGS_LIST["scan-bm"]="--app_rocksdb_enable 1"
WORKLOAD_ARGS_LIST["get-1"]="--app_memcached_enable 1 --app_memsize ${APP_MEMSIZE}  --app_key_range ${SCAN_KEYSPACE_MEMCACHED} --app_key_size ${SCAN_KEYLEN} --app_value_size ${SCAN_VALLEN}"
WORKLOAD_ARGS_LIST["get-2"]="--app_memcached_enable 1 --app_memsize ${APP_MEMSIZE}  --app_key_range ${SCAN_KEYSPACE_MEMCACHED2} --app_key_size ${SCAN_KEYLEN} --app_value_size ${SCAN_VALLEN2}"
WORKLOAD_ARGS_LIST["scan-sjk"]="--app_rocksdb_enable 1 --app_key_range 5000 --app_key_size 16 --app_value_size 32"
WORKLOAD_ARGS_LIST["tpcc"]="--app_silo_enable 1 --app_memsize 40 --app_num_warehouses 200"
WORKLOAD_ARGS_LIST["vsearch"]="--app_faiss_path /mnt/faiss/db.index"

WORKLOAD_ARGS=${WORKLOAD_ARGS_LIST[${WORKLOAD}]}

case "$WORKLOAD" in
"scan-1" | "scan-100" | "scan-bm" | "scan-sjk")
    MOUNT_MNT="rocksdb"
    ;;
"vsearch")
    MOUNT_MNT="faiss"
    ;;
*)
    MOUNT_MNT=""
    ;;
esac

declare -A LOADGEN_EXTRA_ARGS_LIST

LOADGEN_EXTRA_ARGS_LIST["echo"]="--workload ping"
LOADGEN_EXTRA_ARGS_LIST["array-1"]="--workload array64 --wl_app_memsize ${APP_MEMSIZE}"
LOADGEN_EXTRA_ARGS_LIST["warray-1"]="--workload warray64 --wl_app_memsize ${APP_MEMSIZE}"
LOADGEN_EXTRA_ARGS_LIST["array-100"]="--workload array64 --wl_app_memsize ${APP_MEMSIZE} --wl_app_count 100"
LOADGEN_EXTRA_ARGS_LIST["array-bm"]="--workload array64 --wl_app_memsize ${APP_MEMSIZE} --wl_app_count_dist multimodal"
LOADGEN_EXTRA_ARGS_LIST["scan-1"]="--workload scan --wl_app_kspace ${SCAN_KEYSPACE_ROCKSDB} --wl_app_klen ${SCAN_KEYLEN}"
LOADGEN_EXTRA_ARGS_LIST["scan-100"]="--workload scan --wl_app_kspace ${SCAN_KEYSPACE_ROCKSDB} --wl_app_klen ${SCAN_KEYLEN} --wl_app_count 100"
LOADGEN_EXTRA_ARGS_LIST["scan-bm"]="--workload scan --wl_app_kspace ${SCAN_KEYSPACE_ROCKSDB} --wl_app_klen ${SCAN_KEYLEN} --wl_app_count_dist multimodal"
LOADGEN_EXTRA_ARGS_LIST["get-1"]="--workload get --wl_app_memsize ${APP_MEMSIZE} --wl_app_kspace ${SCAN_KEYSPACE_MEMCACHED} --wl_app_klen ${SCAN_KEYLEN}"
LOADGEN_EXTRA_ARGS_LIST["get-2"]="--workload get --wl_app_memsize ${APP_MEMSIZE} --wl_app_kspace ${SCAN_KEYSPACE_MEMCACHED2} --wl_app_klen ${SCAN_KEYLEN}"
LOADGEN_EXTRA_ARGS_LIST["scan-sjk"]="--workload scan --wl_app_kspace 1 --wl_app_klen 10 --wl_app_count_dist multimodal --wl_app_count_multimodal 1,500,5000,500" # Figure 8(b)(c)
LOADGEN_EXTRA_ARGS_LIST["tpcc"]="--workload tpcc --wait 10"
LOADGEN_EXTRA_ARGS_LIST["vsearch"]="--workload vsearch"

LOADGEN_EXTRA_ARGS=${LOADGEN_EXTRA_ARGS_LIST[${WORKLOAD}]}
## Load Generator Configuration (END) ##

## Build Configuration ##

MIMALLOC=mimalloc

BUILD_ROOT="$ROOT_PATH/build"
BUILD_PATH="$BUILD_ROOT/${BUILD_TYPE}"
if [ -n "${BREAKDOWN}" ]; then
    BUILD_PATH="${BUILD_PATH}-breakdown"
fi

RDMA_CORE_PATH="${BUILD_PATH}/rdma-core"
QEMU_PATH="${BUILD_PATH}/qemu"
MIMALLOC_BUILD_PATH="${BUILD_PATH}/mimalloc/"
DISPATCHER_PATH="${BUILD_PATH}/dispatcher"
ROCKSDB_PATH="${BUILD_PATH}/rocksdb"

## Build Configuration (END) ##

### --- CONFIG END --- ###

message() {
    printf "\033[0;32m$1\033[0m\n"
}

install_timeout() {
    rm -f build/bench-sleep
    ln -s /bin/sleep build/bench-sleep
    build/bench-sleep $1 && echo "TIMEOUT!!" && (
        pkill -f $2
    ) &
}

stop_timeout() {
    pkill -f "bench-sleep"
}

### --- Install Functions --- ###

install_apt_common() {
    apt update
    apt install -y python3-pip build-essential gdb bridge-utils numactl \
        gfortran bison pkg-config autoconf automake swig libnl-route-3-200 \
        m4 libltdl-dev ethtool libnl-3-dev debhelper graphviz libnl-3-200 \
        autotools-dev linux-headers-$(uname -r) libnuma1 dkms quilt tk \
        libnl-route-3-dev chrpath dpatch flex tcl libpci-dev libboost-all-dev \
        unzip liblua5.3-dev lua5.3 libssl-dev pax-utils libyaml-cpp-dev \
        openjdk-11-jdk-headless libedit-dev libglib2.0-dev libfdt-dev libpixman-1-dev \
        zlib1g-dev libaio-dev libsysfs-dev net-tools python3-numpy libgflags-dev python3-pandas libdb++-dev \
        bc kmod cpio libncurses5-dev libelf-dev dwarves cgroup-tools libblas-dev libevent-dev libatlas-base-dev aria2

    pip3 install -U pip wheel
    pip3 install ninja meson cmake pyarrow
}

install_ubuntu_1804() {
    install_apt_common
    apt install -y libgfortran4 hugepages

}

install_debian_11() {
    install_apt_common
    apt install -y libgfortran5 libhugetlbfs-bin

}

install_ubuntu_2004() {
    install_apt_common
    apt install -y libgfortran5 libhugetlbfs-bin

}

install-deps() {

    source /etc/os-release

    case $ID in
    debian)
        case $VERSION_ID in
        11) install_debian_11 ;;
        *) echo "Unsupported: $ID $VERSION_ID" ;;
        esac
        ;;

    ubuntu)
        case $VERSION_ID in
        18.04) install_ubuntu_1804 ;;
        20.04) install_ubuntu_2004 ;;
        *) echo "Unsupported: $ID $VERSION_ID" ;;
        esac
        ;;

    *)
        echo "Unsupported: $ID"
        ;;
    esac

}
install-ofed_debian11() {
    pushd /opt
    wget -O ofed.tar.gz https://content.mellanox.com/ofed/MLNX_OFED-5.8-2.0.3.0/MLNX_OFED_LINUX-5.8-2.0.3.0-debian11.3-x86_64.tgz
    tar -xvzf ofed.tar.gz
    cd MLNX_OFED_LINUX-5.8-2.0.3.0-debian11.3-x86_64
    ./mlnxofedinstall --all --force --distro debian11.3
    popd
}
install-ofed_ubuntu2004() {
    pushd /opt
    wget -O ofed.tar.gz https://content.mellanox.com/ofed/MLNX_OFED-5.8-4.1.5.0/MLNX_OFED_LINUX-5.8-4.1.5.0-ubuntu20.04-x86_64.tgz
    tar -xvzf ofed.tar.gz
    cd MLNX_OFED_LINUX-5.8-4.1.5.0-ubuntu20.04-x86_64
    ./mlnxofedinstall --add-kernel-support --force
    popd
}

install-ofed() {

    source /etc/os-release

    case $ID in
    debian)
        case $VERSION_ID in
        11) install-ofed_debian11 ;;
        *) echo "Unsupported: $ID $VERSION_ID" ;;
        esac
        ;;

    ubuntu)
        case $VERSION_ID in
        20.04) install-ofed_ubuntu2004 ;;
        *) echo "Unsupported: $ID $VERSION_ID" ;;
        esac
        ;;

    *)
        echo "Unsupported: $ID"
        ;;
    esac

}

download-hermit() {
    pushd /opt
    if [ ! -d hermit-master ]; then
        git clone -b hermit git@github.com:ANLAB-KAIST/dilos.git hermit-master
    fi
    popd
}

install-hermit-kernel() {
    download-hermit
    pushd /opt/hermit-master/linux-5.14-rc5
    cp config .config
    ./build_kernel.sh build
    ./build_kernel.sh install
    popd
}

prepare-rocksdb() {
    ${DISPATCHER_PATH}/rocksdb-create
}


download-bigann() {
    aria2c --dir /mnt -o bigann_base.bvecs.gz ftp://ftp.irisa.fr/local/texmex/corpus/bigann_base.bvecs.gz
    aria2c --dir /mnt -o bigann_gnd.tar.gz ftp://ftp.irisa.fr/local/texmex/corpus/bigann_gnd.tar.gz
    aria2c --dir /mnt -o bigann_query.bvecs.gz ftp://ftp.irisa.fr/local/texmex/corpus/bigann_query.bvecs.gz
    aria2c --dir /mnt -o bigann_learn.bvecs.gz ftp://ftp.irisa.fr/local/texmex/corpus/bigann_learn.bvecs.gz
}

extract-bigann() {
    gzip -dk bigann_base.bvecs.gz
    gzip -dk bigann_query.bvecs.gz
    gzip -dk bigann_learn.bvecs.gz
}

prepare-bigann() {
    ${DISPATCHER_PATH}/faiss-slice /mnt/bigann_base.bvecs /mnt/bigann_base_100M.bvecs 100
    ${DISPATCHER_PATH}/faiss-rm-hdr /mnt/bigann_query.bvecs /mnt/bigann_learn.bvecs /mnt/bigann_base_100M.bvecs
    mkdir -p /mnt/faiss
    ${DISPATCHER_PATH}/faiss-create /mnt /mnt/faiss
}

prepare-disk() {

    if [ -z "$1" ]; then
        echo "no arg prepare-disk ??"
        exit 1
    fi

    message "Building MNT ($1)"
    echo "/$1/**: /mnt/$1/**" >"/mnt/mnt-$1.manifest"
    ./dilos/scripts/gen-rofs-img.py -o "/mnt/mnt-$1.raw" -m "/mnt/mnt-$1.manifest"
}

download-dataset() {
    download-bigann
}

update-hermit-grub() {
    sed -i 's/GRUB_DEFAULT=.*/GRUB_DEFAULT="Advanced options for Ubuntu>Ubuntu, with Linux 5.14-rc5"/g' /etc/default/grub
    sed -i 's/GRUB_CMDLINE_LINUX=.*/GRUB_CMDLINE_LINUX="transparent_hugepage=madvise"/g' /etc/default/grub
    update-grub
}

update-infiniswap-grub() {
    sed -i 's/GRUB_DEFAULT=.*/GRUB_DEFAULT="Advanced options for Ubuntu>Ubuntu, with Linux 4.11.0-041100-generic"/g' /etc/default/grub
    sed -i 's/GRUB_CMDLINE_LINUX=.*/GRUB_CMDLINE_LINUX="transparent_hugepage=madvise"/g' /etc/default/grub
    update-grub
}

build-hermit-client() {
    pushd 3rdparty/remoteswap/client
    make
    popd
}

build-hermit-server() {
    pushd 3rdparty/remoteswap/server
    # fix compile bug
    make
    popd
}

run-hermit-server() {
    pkill -f rswap-server || true
    sleep 1
    today=$(date +%F_%H-%M-%S)
    pushd 3rdparty/remoteswap/server
    nohup stdbuf -oL numactl -N $MS_NODE -m $MS_NODE ./rswap-server "${MS_RDMA_IP}" "${MS_PORT}" "${MS_MEMORY_GB}" 30 >"$today.log" 2>&1 &
    popd
}
run-hermit-client() {
    today=$(date +%F_%H-%M-%S)
    pushd 3rdparty/remoteswap/client
    SWAP_PARTITION_SIZE_GB=${MS_MEMORY_GB} ./manage_rswap_client.sh install
    popd

    return
    echo 6 >/sys/kernel/debug/hermit/sthd_cnt # number of page reclamation threads. The default cores these threads run on are set in the kernel (https://github.com/ivanium/linux-5.14-rc5/blob/separate-swapout-codepath/mm/hermit.c#L114)

    return

    echo Y >/sys/kernel/debug/hermit/vaddr_swapout # to reduce the overhead of reverse mapping. We record a PA->VA mapping manually for now
    # echo Y >/sys/kernel/debug/hermit/batch_swapout    # batched paging out dirty pages in the page reclamation
    echo Y >/sys/kernel/debug/hermit/bypass_swapcache # for pages that are not shared by multiple PTEs, we can skip adding it into the swap cache and map it directly to the faulting PTE
    echo Y >/sys/kernel/debug/hermit/speculative_io   # speculative RDMA read
    echo Y >/sys/kernel/debug/hermit/speculative_lock # speculative mmap_lock. Mostly useful for java applications
}

### --- Install Functions (END) --- ###

### --- Setup Functions --- ###
setup-compute() {
    message "Setup Bridge"
    brctl addbr dilosbr
    brctl addif dilosbr ${ETH_IF}
    brctl show
    ip addr add ${GW}/${PREFIX} dev dilosbr
    ip link set ${ETH_IF} up
    ip link set dilosbr up
    sysctl -w net.ipv4.ip_forward=1
    sysctl -w kernel.watchdog_thresh=60

    message "Setup RDMA"
    ip addr add ${RDMA_IP}/${PREFIX} dev ${RDMA_IF}
    ip link set ${RDMA_IF} up

    ip addr add ${RDMA_IP_RAW}/${PREFIX} dev ${RDMA_IF_RAW}
    ip link set ${RDMA_IF_RAW} up

    # message "Disable NUMA 1"

    # for core in $(seq 12 23); do
    #     echo 0 >/sys/devices/system/cpu/cpu$core/online
    # done

}

setup-remote() {
    message "Setup IP"
    ip addr add ${MS_IP}/${PREFIX} dev ${MS_ETH_IF}
    ip link set ${MS_ETH_IF} up

    message "Setup HugeTLB"
    hugeadm --pool-pages-min 2M:${MS_HUGE_TLBS}
    hugeadm --create-mounts

    message "Setup RDMA"
    ip addr add ${MS_RDMA_IP}/${PREFIX} dev ${MS_RDMA_IF}
    ip link set ${MS_RDMA_IF} up

}

setup-loadgen() {
    message "Setup RDMA"
    ip addr add ${LG_RDMA_IP}/${PREFIX} dev ${LG_RDMA_IF}
    ip link set ${LG_RDMA_IF} up

}
### --- Setup Functions END --- ###

### --- Build Functions --- ###

build-rdma-core() {
    message "Building RDMA-Core"
    if [ ! -d "$RDMA_CORE_PATH" ] || [ ! -z $RDMA_CORE_FORCE ]; then
        mkdir -p $RDMA_CORE_PATH
        pushd $RDMA_CORE_PATH
        cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DIN_PLACE=1 -DENABLE_STATIC=1 -DENABLE_RESOLVE_NEIGH=0 \
            -DENABLE_VALGRIND=0 -DIOCTL_MODE=write -DNO_PYVERBS=1 -GNinja "${ROOT_PATH}/rdma-core"
        popd
    fi
    pushd $RDMA_CORE_PATH
    ninja
    popd
}

build-qemu() {
    message "Building QEMU"
    if [ ! -d "$QEMU_PATH" ] || [ ! -z $QEMU_FORCE ]; then
        mkdir -p $QEMU_PATH
        pushd $QEMU_PATH
        "${ROOT_PATH}/qemu/configure" --prefix="${QEMU_PATH}" --extra-cflags="-I ${RDMA_CORE_PATH}/include" --target-list=x86_64-softmmu --enable-linux-aio --disable-rdma
        popd
    fi
    pushd $QEMU_PATH
    make -j >${ROOT_PATH}/build/qemu.log
    popd
}

build-mimalloc() {
    message "Building mimalloc"
    if [ ! -d "$MIMALLOC_BUILD_PATH" ] || [ ! -z $MIMALLOC_FORCE ]; then
        mkdir -p $MIMALLOC_BUILD_PATH
        pushd $MIMALLOC_BUILD_PATH
        cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DMI_DDC="${ROOT_PATH}/dilos/include" \
            -DMI_OVERRIDE=OFF -DMI_INTERPOSE=OFF -DMI_BUILD_OBJECT=OFF -DMI_BUILD_SHARED=OFF -DMI_BUILD_TESTS=OFF \
            -GNinja ${MIMALLOC_OPTION} "${ROOT_PATH}/${MIMALLOC}"
        popd
    fi
    pushd $MIMALLOC_BUILD_PATH
    ninja
    popd

}

build-dispatcher() {
    message "Building Dispatcher"
    OPTION_ARGS=""
    if [ -n "${BREAKDOWN}" ]; then
        OPTION_ARGS="${OPTION_ARGS} -DPROTOCOL_BREAKDOWN=ON"
    fi
    if [ -n "${ENABLE_SILO}" ]; then
        OPTION_ARGS="${OPTION_ARGS} -DENABLE_SILO=ON"
    fi
    if [ -n "${ENABLE_FAISS}" ]; then
        OPTION_ARGS="${OPTION_ARGS} -DENABLE_FAISS=ON"
    fi

    OPTION_TXT="${DISPATCHER_PATH}/option.txt"
    if [ ! -d "$DISPATCHER_PATH" ] || [ ! -z $DISPATCHER_FORCE ] || [ "$OPTION_ARGS" != "$(cat ${OPTION_TXT})" ]; then
        mkdir -p $DISPATCHER_PATH
        pushd $DISPATCHER_PATH

        echo "${OPTION_ARGS}" >"${OPTION_TXT}"

        cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} ${OPTION_ARGS} -GNinja "${ROOT_PATH}/dispatcher"
        popd
    fi
    pushd $DISPATCHER_PATH
    ninja
    popd
}

build-mnt() {
    message "Building MNT"
    echo "/**: /mnt/**" >"${BUILD_ROOT}/mnt.manifest"
    ./dilos/scripts/gen-rofs-img.py -o "${BUILD_ROOT}/mnt.raw" -m "${BUILD_ROOT}/mnt.manifest"

}

build() {
    message "ROOT_PATH: $ROOT_PATH"
    message "MIMALLOC_BUILD_PATH: $MIMALLOC_BUILD_PATH"

    build-rdma-core

    build-qemu

    build-mimalloc

    build-dispatcher

    message "Building DiLOS"
    pushd dilos
    if [[ -n "$*" ]]; then
        IMAGE="$*"
    else
        IMAGE=dp-run
    fi

    export QEMU_NBD=${QEMU_PATH}/qemu-nbd
    export QEMU_IMG=${QEMU_PATH}/qemu-img
    OPTION_ARGS=""
    if [ -n "${ENABLE_SILO}" ]; then
        OPTION_ARGS="${OPTION_ARGS} ENABLE_SILO=ON"
    fi
    if [ -n "${ENABLE_FAISS}" ]; then
        OPTION_ARGS="${OPTION_ARGS} ENABLE_FAISS=ON"
    fi

    ./scripts/build mode=release -j DILOS_ROOT="$ROOT_PATH" ${OPTION_ARGS} DILOS_BUILD_PATH="${BUILD_PATH}" fs=rofs image=$IMAGE

    popd

}

clean-app() {
    for d in apps/*/; do
        pushd $d || exit
        make clean >/dev/null
        popd || exit
    done
}

clean() {
    pushd dilos
    make clean
    popd

    rm -r build || true

}
### --- Build Functions END --- ###

### --- Run Functions --- ###

run() {

    if [[ -n $TRACE ]]; then
        TRACE_ARGS="--trace=${TRACE}"
    fi

    set +x
    NUMA_ARG=""
    for affinity in $(seq $CPU_START $CPU_LAST); do
        vcpu=$((affinity - CPU_START))
        NUMA_ARG="${NUMA_ARG} --pass-args=-vcpu --pass-args=vcpunum=$vcpu,affinity=$affinity"
    done
    set -x

    if [[ -n $VERBOSE ]]; then
        VERBOSE_ARG="--verbose"
    fi
    if [[ -z $NO_NETWORK ]]; then
        NETWORK_ARGS="-n -b dilosbr --vhost"
    fi

    if [[ -n $MOUNT_MNT ]]; then
        DISK_HD=hd1
        DISK_BLK=vblk1
        DISK_IMG_OPT="--pass-args=-device --pass-args=virtio-blk-${PCI},id=${DISK_BLK},drive=${DISK_HD},scsi=off --pass-args=-drive --pass-args=file=/mnt/mnt-${MOUNT_MNT}.raw,if=none,id=${DISK_HD},cache=none,aio=native "
        DISK_MOUNT_CMD="--mount-fs=rofs,/dev/${DISK_BLK},/mnt"
    fi
    if [[ -n "${WAIT}" ]]; then
        WAIT_ARGS="--wait"
    fi

    RUN_ARGS="${VERBOSE_ARG} \
        --qemu-path ${QEMU_PATH}/x86_64-softmmu/qemu-system-x86_64 \
        -p ${HYPERVISOR} \
        -c ${TOTAL_CPUS} \
        ${NUMA_ARG} \
        -m ${MEMORY} \
        ${WAIT_ARGS} \
        ${NETWORK_ARGS} \
        --pass-args=-device --pass-args=virtio-uverbs-${PCI},host=${IB_UVERBS} \
        --pass-args=-device --pass-args=virtio-uverbs-${PCI},host=${ETH_UVERBS} \
        ${DISK_IMG_OPT} \
        ${TRACE_ARGS} "

    if [[ -n "${DDC_DISABLE}" ]]; then
        DDC_DISABLE_CMD="--ddc_disable=1"
    fi

    NETWORK_CMD="--ip=eth0,${IP},${SUBNET} --defaultgw=${GW} --nameserver=${NAMESERVER}"

    if [[ -n "${PREFETCHER}" ]]; then
        PREFETCHER_CMD="--prefetcher=${PREFETCHER}"
    fi

    if [[ "${AUTO_POWEROFF}" = "y" ]]; then
        POWEROFF_CMD="--power-off-on-abort"
    else
        POWEROFF_CMD="--noshutdown"
    fi

    if [[ -n "${PIN}" ]]; then
        PIN_CMD="--cpu_pin=${PIN}"
    fi

    CMD_PREFIX="${NETWORK_CMD} ${DDC_DISABLE_CMD} ${IB_CMD} ${PREFETCHER_CMD} ${DISK_MOUNT_CMD} ${POWEROFF_CMD} ${PIN_CMD} --disable_rofs_cache "

    if [[ -n "$*" ]]; then
        CMDLINE="$*"
    else
        CMDLINE=$(cat dilos/build/last/cmdline)
    fi

    export QEMU_NBD=${QEMU_PATH}/qemu-nbd
    export QEMU_IMG=${QEMU_PATH}/qemu-img

    ./dilos/scripts/run.py ${RUN_ARGS} -e "${CMD_PREFIX} ${CMDLINE}"
}

DISPATCHER_PARAM="\
--eth_dev_name ${ETH_DEVICE} \
--ib_dev_name ${IB_DEVICE} \
--worker_core_id_start ${WORKER_CPU_START} \
--worker_core_num ${WORKER_CPUS} \
${WORKLOAD_ARGS}"

run-native() {
    cd "${DISPATCHER_PATH}"
    numactl -N $NODE -m $NODE ./dispatcher_linux --dp_mode 0 --pf_mode 0 ${DISPATCHER_PARAM} $@
}

run-native-hermit() {
    mkdir -p /sys/fs/cgroup/memory/hermit
    echo ${MEMORY} >/sys/fs/cgroup/memory/hermit/memory.limit_in_bytes
    cd "${DISPATCHER_PATH}"
    stdbuf -oL cgexec --sticky -g memory:hermit numactl -N $NODE -m $NODE ./dispatcher_linux --dp_mode 0 --pf_mode 0 ${DISPATCHER_PARAM} $@
}

run-vm-hybrid() {
    run /dp-run --dp_mode 0 --pf_mode 3 ${DISPATCHER_PARAM} --server_hostname ${MS_IP} --server_port ${MS_PORT} $@
}
run-vm-async() {
    run /dp-run --dp_mode 0 --pf_mode 2 ${DISPATCHER_PARAM} --server_hostname ${MS_IP} --server_port ${MS_PORT} $@
}
run-vm-sync() {
    run /dp-run --dp_mode 0 --pf_mode 1 ${DISPATCHER_PARAM} --server_hostname ${MS_IP} --server_port ${MS_PORT} $@
}
run-vm-shinjuku() {
    # for 5us in 2GHz processor: 10,000
    run /dp-run --dp_mode 0 --pf_mode 5 --preemption_cycles 10000 ${DISPATCHER_PARAM} --server_hostname ${MS_IP} --server_port ${MS_PORT} $@
}
run-vm-shinjuku-custom() {
    CYCLES=$1
    shift
    # for 5us in 2GHz processor: 10,000
    run /dp-run --dp_mode 0 --pf_mode 5 --preemption_cycles "${CYCLES}" ${DISPATCHER_PARAM} --server_hostname ${MS_IP} --server_port ${MS_PORT} $@
}
run-vm-shinjuku2-custom() {
    CYCLES=$1
    shift
    # for 5us in 2GHz processor: 10,000
    run /dp-run --dp_mode 0 --pf_mode 6 --preemption_cycles "${CYCLES}" ${DISPATCHER_PARAM} --server_hostname ${MS_IP} --server_port ${MS_PORT} $@
}
run-vm-local-shinjuku2-custom() {
    CYCLES=$1
    shift
    # for 5us in 2GHz processor: 10,000
    MEMORY=32G run /dp-run --dp_mode 0 --pf_mode 7 --preemption_cycles "${CYCLES}" ${DISPATCHER_PARAM} $@
}
run-vm-async-mq() {
    run /dp-run --dp_mode 1 --pf_mode 2 ${DISPATCHER_PARAM} --server_hostname ${MS_IP} --server_port ${MS_PORT} $@
}
run-vm-hybrid-mq() {
    run /dp-run --dp_mode 1 --pf_mode 3 ${DISPATCHER_PARAM} --server_hostname ${MS_IP} --server_port ${MS_PORT} $@
}
run-vm-async-local() {
    run /dp-run --dp_mode 0 --pf_mode 4 ${DISPATCHER_PARAM} --server_hostname ${MS_IP} --server_port ${MS_PORT} $@
}

run-vm-async-local-sched1() {
    run /dp-run --dp_mode 0 --pf_mode 4 --sched_mode 1 ${DISPATCHER_PARAM} --server_hostname ${MS_IP} --server_port ${MS_PORT} $@
}

run-vm-local() {
    MEMORY=20G run /dp-run --dp_mode 0 --pf_mode 0 ${DISPATCHER_PARAM} $@
}

run-mem-server() {
    today=$(date +%F_%H-%M-%S)
    cd "${DISPATCHER_PATH}"
    nohup stdbuf -oL numactl -N $MS_NODE -m $MS_NODE ./mem-server $@ >"$today.log" 2>&1 &
}
kill-mem-server() {
    pkill -f "./mem-server"
    sleep 2
}

kill-mem-server-all() {
    pkill -f "./mem-server"
    pkill -f "rswap-server"
    sleep 2
}
wait-mem-server() {
    while ! nc -z "${MS_IP}" "${MS_PORT}"; do
        sleep 2
    done
}

debug() {
    cd dilos
    gdb -iex "set auto-load safe-path ." -ex connect -ex "osv syms" build/release/loader.elf 2>/dev/null
}

trace() {
    cd dilos
    gdb -batch -iex "set auto-load safe-path ." \
        -ex "connect" \
        -ex "osv syms" \
        -ex "set pagination off" \
        -ex "osv trace" \
        build/release/loader.elf >"${ROOT_PATH}/build/trace.log"
}
kill-dispatcher-native() {
    pkill -f dispatcher_linux
    sleep 2

}
kill-qemu() {
    pkill -f qemu-system-x86_64
    sleep 2
}

### --- Run Functions END --- ###

### --- Remote Functions  --- ###
run-remote() {
    ssh $MS_SSH "export BREAKDOWN=${BREAKDOWN}; $@"
}
run-remote-lg() {
    ssh $LG_SSH "export BREAKDOWN=${BREAKDOWN}; $@"

}

remote-build() {
    run-remote "cd $ROOT_PATH; ./launch.sh build-dispatcher"

    if [ "${MS_SSH}" != "${LG_SSH}" ]; then
        run-remote-lg "cd $ROOT_PATH; ./launch.sh build-dispatcher"
    fi
}
remote-clean() {
    run-remote "cd $ROOT_PATH; ./launch.sh clean"
    if [ "${MS_SSH}" != "${LG_SSH}" ]; then
        run-remote-lg "cd $ROOT_PATH; ./launch.sh clean"
    fi
}
remote-setup() {
    run-remote "cd $ROOT_PATH; ./launch.sh setup-remote"
}
loadgen-setup() {
    run-remote-lg "cd $ROOT_PATH; ./launch.sh setup-loadgen"
}

remote-up() {
    run-remote "cd $ROOT_PATH; ./launch.sh run-mem-server \
        --dev_name \"${MS_DEVNAME}\" \
        --memory_size_gb \"${MS_MEMORY_GB}\" \
        --tcp_port_number \"${MS_PORT}\"      
        "
}

remote-down() {
    run-remote "cd $ROOT_PATH; ./launch.sh kill-mem-server"
}

run-loadgen() {
    cd "${DISPATCHER_PATH}"
    numactl -N $LG_NODE -m $LG_NODE ./loadgen $@
}

LOADGEN_PARAM="--out_path ${LG_OUTPUT} \
        --dev_name ${LG_DEVNAME} \
        --ib_port ${LG_PORT} \
        --ib_num_rx_cq_desc ${LG_RX_CQ} \
        --ib_num_rx_wr_desc ${LG_RX_WR} \
        --core_id_start ${LG_CORE_START} \
        --core_num 0 \
        --num_operation ${LG_NUM_OPERATION} \
        --duration ${LG_DURATION} \
        --rps ${LG_RPS} \
        ${LOADGEN_EXTRA_ARGS}"

loadgen-remote() {
    run-remote-lg "mkdir -p \"${LG_OUTPUT}\""
    run-remote-lg "cd $ROOT_PATH; ./launch.sh run-loadgen ${LOADGEN_PARAM} $@ > ${LG_OUTPUT}/loadgen.log  2>&1 "

}
loadgen-local() {
    mkdir -p "${LG_OUTPUT}"
    run-loadgen ${LOADGEN_PARAM} $@
}

sync() {
    rsync -avzh \
        --exclude ".git" \
        --exclude "build/" \
        "${ROOT_PATH}/" "${MS_SSH}:${ROOT_PATH}"

    if [ "${MS_SSH}" != "${LG_SSH}" ]; then
        rsync -avzh \
            --exclude ".git" \
            --exclude "build/" \
            "${ROOT_PATH}/" "${LG_SSH}:${ROOT_PATH}"
    fi

}

wait-dispatcher() {
    LOGFILE="$1"

    while ! grep "Running Main Loop" "${LOGFILE}"; do
        sleep 2
    done

}
exp-cleanup() {
    kill-dispatcher-native || true
    kill-qemu || true
    remote-down || true
}
exp-prepare() {
    exp-cleanup
    sync
    remote-build
    build
    remote-up
    wait-mem-server

}

do-exp() {
    set +x
    local experiment=""
    local workload=""
    local start="100"
    local step="100"
    local end="200"
    local num_cpus="$WORKER_CPUS"
    local memory_size="$MEMORY"
    local num_try="1"

    local do_fast=""
    local do_native=""

    declare -A modes

    while [[ "$#" -gt 0 ]]; do
        case $1 in
        --experiment | --exp)
            experiment="$2"
            shift 2
            ;;
        --workload)
            workload="$2"
            shift 2
            ;;
        --start)
            start="$2"
            shift 2
            ;;
        --step)
            step="$2"
            shift 2
            ;;
        --num_cpus)
            num_cpus="$2"
            shift 2
            ;;
        --memory_size)
            memory_size="$2"
            shift 2
            ;;
        --num_try)
            num_try="$2"
            shift 2
            ;;
        --mode)
            local rps=$3
            rps="${rps//k/000}"
            rps="${rps//K/000}"
            modes[$2]=$rps
            shift 3
            ;;
        --fast)
            do_fast="1"
            shift 1
            ;;
        --native)
            do_native="1"
            shift 1
            ;;
        *)
            echo "Unknown parameter passed: $1"
            exit 1
            ;;
        esac
    done

    if [ -z "$experiment" ]; then
        echo "no experiment"
        set -x
        retrun 1
    fi
    if [ -z "$workload" ]; then
        echo "no workload"
        set -x
        retrun 1
    fi
    BENCHMARK_OUT_THIS="${BENCHMARK_OUT}/${experiment}/${TODAY}"

    start="${start//k/000}"
    start="${start//K/000}"
    step="${step//k/000}"
    step="${step//K/000}"

    for value in "${modes[@]}"; do
        if ((value > end)); then
            end=$value
        fi
    done

    mkdir -p "${BENCHMARK_OUT_THIS}"

    echo "Workload: $workload, Experiment: $experiment"
    echo "Start: $start, Step: $step, End: $end"

    set -x
    exp-cleanup

    if [ -z "$do_fast" ]; then

        for try in $(seq 1 $num_try); do
            for rps in $(seq "$start" "$step" "$end"); do
                for mode in ${!modes[@]}; do
                    if ((rps > ${modes[${mode}]})); then
                        continue
                    fi
                    echo "Mode: ${mode} Exp: ${experiment} Workload: ${workload} MEMORY_SIZE: ${memory_size} rps: ${rps} try: ${try}"
                    OUT_PATH="${BENCHMARK_OUT_THIS}/${mode}_${rps}rps_${try}"
                    mkdir -p ${OUT_PATH}
                    if [ -z "$do_native" ]; then
                        exp-prepare
                    fi

                    DISPATCHER_LOG="${OUT_PATH}/dispatcher.log"
                    WORKLOAD=${workload} WORKER_CPUS=${num_cpus} MEMORY=${memory_size} POWEROFF_CMD=y nohup stdbuf -oL ./launch.sh ${mode} >"${DISPATCHER_LOG}" 2>&1 &
                    wait-dispatcher "${DISPATCHER_LOG}"
                    sleep 2 # for safety
                    WORKLOAD=${workload} LG_RPS=$rps LG_OUTPUT="${OUT_PATH}" ./launch.sh lr
                    exp-cleanup
                    ./launch.sh benchmark-upload || true
                    ./launch.sh remote-benchmark-upload || true
                    if [ -n "$do_native" ]; then
                        sleep 120 # wait for reclaim
                    fi
                done
            done
        done
    else
        # do fast
        for try in $(seq 1 $num_try); do
            for mode in ${!modes[@]}; do
                if [ -z "$do_native" ]; then
                    exp-prepare
                fi
                DISPATCHER_LOG="${BENCHMARK_OUT_THIS}/dispatcher-${mode}-${try}.log"
                WORKLOAD=${workload} WORKER_CPUS=${num_cpus} MEMORY=${memory_size} POWEROFF_CMD=y nohup ./launch.sh ${mode} >"${DISPATCHER_LOG}" 2>&1 &
                wait-dispatcher "${DISPATCHER_LOG}"
                sleep 2 # for safety

                for rps in $(seq "$start" "$step" "$end"); do
                    if ((rps > ${modes[${mode}]})); then
                        continue
                    fi
                    sleep 60 # for safety
                    echo "Mode: ${mode} Exp: ${experiment} Workload: ${workload} MEMORY_SIZE: ${memory_size} rps: ${rps} try: ${try}"
                    OUT_PATH="${BENCHMARK_OUT_THIS}/${mode}_${rps}rps_${try}"
                    mkdir -p ${OUT_PATH}
                    WORKLOAD=${workload} LG_RPS=$rps LG_OUTPUT="${OUT_PATH}" ./launch.sh lr
                    ./launch.sh benchmark-upload || true
                    ./launch.sh remote-benchmark-upload || true
                    if [ -n "$do_native" ]; then
                        sleep 120 # wait for reclaim
                    fi
                done
                exp-cleanup
            done
        done
    fi
}

declare -A MODES
exp-repeat-rps() {
    EXP=$1
    WORKLOAD=$2
    START_RPS=$3
    MAX_RPS=$4
    STEP_RPS=$5
    NUM_CPUS=$6
    MEMORY_SIZE=$7
    NUM_TRY=$8
    set +x
    if [ -z "$EXP" ]; then
        echo "no EXP"
        retrun 1
    fi
    if [ -z "$WORKLOAD" ]; then
        echo "no WORKLOAD"
        retrun 1
    fi
    if [ -z "$START_RPS" ]; then
        echo "no START_RPS"
        retrun 1
    fi
    if [ -z "$MAX_RPS" ]; then
        echo "no MAX_RPS"
        retrun 1
    fi
    if [ -z "$NUM_TRY" ]; then
        NUM_TRY=1
    fi
    if [ -z "$STEP_RPS" ]; then
        STEP_RPS=100
    fi
    if [ -z "$MEMORY_SIZE" ]; then
        MEMORY_SIZE=$MEMORY
    fi
    if [ -z "$NUM_CPUS" ]; then
        NUM_CPUS=1
    fi
    set +x

    BENCHMARK_OUT_THIS="${BENCHMARK_OUT}/${EXP}/${TODAY}"
    for try in $(seq 1 $NUM_TRY); do
        for rps in $(seq $START_RPS $STEP_RPS $MAX_RPS); do
            for MODE in ${!MODES[@]}; do
                if ((rps > ${MODES[${MODE}]})); then
                    continue
                fi
                exp-prepare
                echo "Mode: ${MODE} Exp: ${EXP} Workload: ${WORKLOAD} MEMORY_SIZE: ${MEMORY_SIZE} rps: ${rps} try: ${try}"
                set -x
                OUT_PATH="${BENCHMARK_OUT_THIS}/${MODE}_${rps}rps_${try}"
                mkdir -p ${OUT_PATH}

                DISPATCHER_LOG="${OUT_PATH}/dispatcher.log"
                WORKLOAD=${WORKLOAD} WORKER_CPUS=${NUM_CPUS} MEMORY=${MEMORY_SIZE} POWEROFF_CMD=y nohup ./launch.sh ${MODE} >"${DISPATCHER_LOG}" 2>&1 &
                wait-dispatcher "${DISPATCHER_LOG}"
                sleep 2 # for safety
                WORKLOAD=${WORKLOAD} LG_RPS=$rps LG_OUTPUT="${OUT_PATH}" ./launch.sh lr
                exp-cleanup
                ./launch.sh benchmark-upload || true
                ./launch.sh remote-benchmark-upload || true

            done
        done
    done
}

exp-repeat() {
    EXP=$1
    WORKLOAD=$2
    START_KRPS=$3
    MAX_KRPS=$4
    STEP_KRPS=$5
    NUM_CPUS=$6
    MEMORY_SIZE=$7
    NUM_TRY=$8
    set +x
    if [ -z "$EXP" ]; then
        echo "no EXP"
        retrun 1
    fi
    if [ -z "$WORKLOAD" ]; then
        echo "no WORKLOAD"
        retrun 1
    fi
    if [ -z "$START_KRPS" ]; then
        echo "no START_KRPS"
        retrun 1
    fi
    if [ -z "$MAX_KRPS" ]; then
        echo "no MAX_KRPS"
        retrun 1
    fi
    if [ -z "$NUM_TRY" ]; then
        NUM_TRY=1
    fi
    if [ -z "$STEP_KRPS" ]; then
        STEP_KRPS=100
    fi
    if [ -z "$MEMORY_SIZE" ]; then
        MEMORY_SIZE=$MEMORY
    fi
    if [ -z "$NUM_CPUS" ]; then
        NUM_CPUS=1
    fi
    set -x

    BENCHMARK_OUT_THIS="${BENCHMARK_OUT}/${EXP}/${TODAY}"
    for try in $(seq 1 $NUM_TRY); do
        for krps in $(seq $START_KRPS $STEP_KRPS $MAX_KRPS); do
            for MODE in ${!MODES[@]}; do
                if ((krps > ${MODES[${MODE}]})); then
                    continue
                fi
                echo "Mode: ${MODE} Exp: ${EXP} Workload: ${WORKLOAD} MEMORY_SIZE: ${MEMORY_SIZE} krps: ${krps} try: ${try}"
                set -x
                OUT_PATH="${BENCHMARK_OUT_THIS}/${MODE}_${krps}krps_${try}"
                mkdir -p ${OUT_PATH}
                exp-prepare
                DISPATCHER_LOG="${OUT_PATH}/dispatcher.log"
                WORKLOAD=${WORKLOAD} WORKER_CPUS=${NUM_CPUS} MEMORY=${MEMORY_SIZE} POWEROFF_CMD=y nohup ./launch.sh ${MODE} >"${DISPATCHER_LOG}" 2>&1 &
                wait-dispatcher "${DISPATCHER_LOG}"
                sleep 2 # for safety
                rps="${krps}000"
                WORKLOAD=${WORKLOAD} LG_RPS=$rps LG_OUTPUT="${OUT_PATH}" ./launch.sh lr
                exp-cleanup
                ./launch.sh benchmark-upload || true
                ./launch.sh remote-benchmark-upload || true
            done
        done
    done
}

exp-repeat-mem() {
    EXP=$1
    WORKLOAD=$2
    START_KRPS=$3
    MAX_KRPS=$4
    STEP_KRPS=$5
    NUM_CPUS=$6
    NUM_TRY=$7
    set +x
    if [ -z "$EXP" ]; then
        echo "no EXP"
        retrun 1
    fi
    if [ -z "$WORKLOAD" ]; then
        echo "no WORKLOAD"
        retrun 1
    fi
    if [ -z "$START_KRPS" ]; then
        echo "no START_KRPS"
        retrun 1
    fi
    if [ -z "$MAX_KRPS" ]; then
        echo "no MAX_KRPS"
        retrun 1
    fi
    if [ -z "$NUM_TRY" ]; then
        NUM_TRY=1
    fi
    if [ -z "$STEP_KRPS" ]; then
        STEP_KRPS=100
    fi
    if [ -z "$NUM_CPUS" ]; then
        NUM_CPUS=1
    fi
    set -x

    BENCHMARK_OUT_THIS="${BENCHMARK_OUT}/${EXP}/${TODAY}"
    mkdir -p "${BENCHMARK_OUT_THIS}"
    for MEMORY_SIZE in "${MEMORY_SIZES[@]}"; do
        for try in $(seq 1 $NUM_TRY); do
            for MODE in ${!MODES[@]}; do
                exp-prepare
                DISPATCHER_LOG="${BENCHMARK_OUT_THIS}/dispatcher.log"
                WORKLOAD=${WORKLOAD} WORKER_CPUS=${NUM_CPUS} POWEROFF_CMD=y MEMORY=${MEMORY_SIZE} nohup ./launch.sh ${MODE} >"${DISPATCHER_LOG}" 2>&1 &
                wait-dispatcher "${DISPATCHER_LOG}"
                sleep 2 # for safety

                for krps in $(seq $START_KRPS $STEP_KRPS $MAX_KRPS); do

                    if ((krps > ${MODES[${MODE}]})); then
                        continue
                    fi

                    sleep 10 #for safety
                    echo "Mode: ${MODE} Memory: ${MEMORY_SIZE} Exp: ${EXP} Workload: ${WORKLOAD} krps: ${krps} try: ${try}"
                    set -x
                    OUT_PATH="${BENCHMARK_OUT_THIS}/${MODE}_${krps}krps_${MEMORY_SIZE}_${try}"
                    mkdir -p ${OUT_PATH}

                    rps="${krps}000"
                    WORKLOAD=${WORKLOAD} LG_RPS=$rps LG_OUTPUT="${OUT_PATH}" ./launch.sh lr
                    exp-cleanup
                    ./launch.sh benchmark-upload || true
                    ./launch.sh remote-benchmark-upload || true
                done
            done
        done
    done
}

exp-repeat-local() {
    EXP=$1
    WORKLOAD=$2
    START_KRPS=$3
    MAX_KRPS=$4
    STEP_KRPS=$5
    NUM_CPUS=$6
    MEMORY_SIZE=$7
    NUM_TRY=$8
    set +x
    if [ -z "$EXP" ]; then
        echo "no EXP"
        retrun 1
    fi
    if [ -z "$WORKLOAD" ]; then
        echo "no WORKLOAD"
        retrun 1
    fi
    if [ -z "$START_KRPS" ]; then
        echo "no START_KRPS"
        retrun 1
    fi
    if [ -z "$MAX_KRPS" ]; then
        echo "no MAX_KRPS"
        retrun 1
    fi
    if [ -z "$NUM_TRY" ]; then
        NUM_TRY=1
    fi
    if [ -z "$STEP_KRPS" ]; then
        STEP_KRPS=100
    fi
    if [ -z "$MEMORY_SIZE" ]; then
        MEMORY_SIZE=$MEMORY
    fi
    if [ -z "$NUM_CPUS" ]; then
        NUM_CPUS=1
    fi
    set +x

    BENCHMARK_OUT_THIS="${BENCHMARK_OUT}/${EXP}/${TODAY}"
    for try in $(seq 1 $NUM_TRY); do
        for krps in $(seq $START_KRPS $STEP_KRPS $MAX_KRPS); do
            for MODE in ${!MODES[@]}; do
                if ((krps > ${MODES[${MODE}]})); then
                    continue
                fi
                echo "Mode: ${MODE} Exp: ${EXP} Workload: ${WORKLOAD} MEMORY_SIZE: ${MEMORY_SIZE} krps: ${krps} try: ${try}"
                set -x
                OUT_PATH="${BENCHMARK_OUT_THIS}/${MODE}_${krps}krps_${try}"
                mkdir -p ${OUT_PATH}
                exp-cleanup
                DISPATCHER_LOG="${OUT_PATH}/dispatcher.log"
                WORKLOAD=${WORKLOAD} WORKER_CPUS=${NUM_CPUS} MEMORY=${MEMORY_SIZE} POWEROFF_CMD=y nohup ./launch.sh ${MODE} >"${DISPATCHER_LOG}" 2>&1 &
                wait-dispatcher "${DISPATCHER_LOG}"
                sleep 2 # for safety
                rps="${krps}000"
                WORKLOAD=${WORKLOAD} LG_RPS=$rps LG_OUTPUT="${OUT_PATH}" ./launch.sh lr
                exp-cleanup
                ./launch.sh benchmark-upload || true
                ./launch.sh remote-benchmark-upload || true
            done
        done
    done
}

exp-repeat-fast-rps() {
    EXP=$1
    WORKLOAD=$2
    START_RPS=$3
    MAX_RPS=$4
    STEP_RPS=$5
    NUM_CPUS=$6
    MEMORY_SIZE=$7
    NUM_TRY=$8
    set +x
    if [ -z "$EXP" ]; then
        echo "no EXP"
        retrun 1
    fi
    if [ -z "$WORKLOAD" ]; then
        echo "no WORKLOAD"
        retrun 1
    fi
    if [ -z "$START_RPS" ]; then
        echo "no START_RPS"
        retrun 1
    fi
    if [ -z "$MAX_RPS" ]; then
        echo "no MAX_RPS"
        retrun 1
    fi
    if [ -z "$NUM_TRY" ]; then
        NUM_TRY=1
    fi
    if [ -z "$STEP_RPS" ]; then
        STEP_RPS=100
    fi
    if [ -z "$MEMORY_SIZE" ]; then
        MEMORY_SIZE=$MEMORY
    fi
    if [ -z "$NUM_CPUS" ]; then
        NUM_CPUS=1
    fi
    set -x

    BENCHMARK_OUT_THIS="${BENCHMARK_OUT}/${EXP}/${TODAY}"
    mkdir -p "${BENCHMARK_OUT_THIS}"
    for try in $(seq 1 $NUM_TRY); do
        for MODE in ${!MODES[@]}; do
            exp-prepare
            DISPATCHER_LOG="${BENCHMARK_OUT_THIS}/dispatcher-${MODE}-${try}.log"
            WORKLOAD=${WORKLOAD} WORKER_CPUS=${NUM_CPUS} MEMORY=${MEMORY_SIZE} POWEROFF_CMD=y nohup ./launch.sh ${MODE} >"${DISPATCHER_LOG}" 2>&1 &
            wait-dispatcher "${DISPATCHER_LOG}"
            sleep 2 # for safety
            for rps in $(seq $START_RPS $STEP_RPS $MAX_RPS); do
                if ((rps > ${MODES[${MODE}]})); then
                    continue
                fi
                sleep 10 # for safety
                echo "Mode: ${MODE} Exp: ${EXP} Workload: ${WORKLOAD} MEMORY_SIZE: ${MEMORY_SIZE} rps: ${rps} try: ${try}"
                OUT_PATH="${BENCHMARK_OUT_THIS}/${MODE}_${rps}rps_${try}"
                mkdir -p ${OUT_PATH}
                WORKLOAD=${WORKLOAD} LG_RPS=$rps LG_OUTPUT="${OUT_PATH}" ./launch.sh lr
                ./launch.sh benchmark-upload || true
                ./launch.sh remote-benchmark-upload || true
            done
            exp-cleanup
        done
    done
}

exp-repeat-fast() {
    exp-repeat-fast-rps $1 $2 $3 "${4}000" $5 $6 $7
}

exp-repeat-hermit() {
    EXP=$1
    WORKLOAD=$2
    START_KRPS=$3
    MAX_KRPS=$4
    STEP_KRPS=$5
    NUM_CPUS=$6
    MEMORY_SIZE=$7
    NUM_TRY=$8
    set +x
    if [ -z "$EXP" ]; then
        echo "no EXP"
        retrun 1
    fi
    if [ -z "$WORKLOAD" ]; then
        echo "no WORKLOAD"
        retrun 1
    fi
    if [ -z "$START_KRPS" ]; then
        echo "no START_KRPS"
        retrun 1
    fi
    if [ -z "$MAX_KRPS" ]; then
        echo "no MAX_KRPS"
        retrun 1
    fi
    if [ -z "$NUM_TRY" ]; then
        NUM_TRY=1
    fi
    if [ -z "$STEP_KRPS" ]; then
        STEP_KRPS=100
    fi
    if [ -z "$MEMORY_SIZE" ]; then
        MEMORY_SIZE=$MEMORY
    fi
    if [ -z "$NUM_CPUS" ]; then
        NUM_CPUS=1
    fi
    set -x
    exp-cleanup
    BENCHMARK_OUT_THIS="${BENCHMARK_OUT}/${EXP}/${TODAY}"
    for try in $(seq 1 $NUM_TRY); do
        for krps in $(seq $START_KRPS $STEP_KRPS $MAX_KRPS); do
            for MODE in ${!MODES[@]}; do
                if ((krps > ${MODES[${MODE}]})); then
                    continue
                fi
                echo "Mode: ${MODE} Exp: ${EXP} Workload: ${WORKLOAD} MEMORY_SIZE: ${MEMORY_SIZE} krps: ${krps} try: ${try}"
                set -x
                OUT_PATH="${BENCHMARK_OUT_THIS}/${MODE}_${krps}krps_${try}"
                mkdir -p ${OUT_PATH}

                DISPATCHER_LOG="${OUT_PATH}/dispatcher.log"
                WORKLOAD=${WORKLOAD} WORKER_CPUS=${NUM_CPUS} MEMORY=${MEMORY_SIZE} POWEROFF_CMD=y nohup stdbuf -oL ./launch.sh ${MODE} >"${DISPATCHER_LOG}" 2>&1 &
                wait-dispatcher "${DISPATCHER_LOG}"
                sleep 2 # for safety
                rps="${krps}000"
                WORKLOAD=${WORKLOAD} LG_RPS=$rps LG_OUTPUT="${OUT_PATH}" ./launch.sh lr
                exp-cleanup
                ./launch.sh benchmark-upload || true
                ./launch.sh remote-benchmark-upload || true
                sleep 120 # wait for reclaim
            done
        done
    done
}

exp-repeat-rps-hermit() {
    EXP=$1
    WORKLOAD=$2
    START_RPS=$3
    MAX_RPS=$4
    STEP_RPS=$5
    NUM_CPUS=$6
    MEMORY_SIZE=$7
    NUM_TRY=$8
    set +x
    if [ -z "$EXP" ]; then
        echo "no EXP"
        retrun 1
    fi
    if [ -z "$WORKLOAD" ]; then
        echo "no WORKLOAD"
        retrun 1
    fi
    if [ -z "$START_RPS" ]; then
        echo "no START_RPS"
        retrun 1
    fi
    if [ -z "$MAX_RPS" ]; then
        echo "no MAX_RPS"
        retrun 1
    fi
    if [ -z "$NUM_TRY" ]; then
        NUM_TRY=1
    fi
    if [ -z "$STEP_RPS" ]; then
        STEP_RPS=100
    fi
    if [ -z "$MEMORY_SIZE" ]; then
        MEMORY_SIZE=$MEMORY
    fi
    if [ -z "$NUM_CPUS" ]; then
        NUM_CPUS=1
    fi
    set -x
    exp-cleanup
    BENCHMARK_OUT_THIS="${BENCHMARK_OUT}/${EXP}/${TODAY}"
    for try in $(seq 1 $NUM_TRY); do
        for rps in $(seq $START_RPS $STEP_RPS $MAX_RPS); do
            for MODE in ${!MODES[@]}; do
                if ((rps > ${MODES[${MODE}]})); then
                    continue
                fi
                echo "Mode: ${MODE} Exp: ${EXP} Workload: ${WORKLOAD} MEMORY_SIZE: ${MEMORY_SIZE} rps: ${rps} try: ${try}"
                set -x
                OUT_PATH="${BENCHMARK_OUT_THIS}/${MODE}_${rps}rps_${try}"
                mkdir -p ${OUT_PATH}

                DISPATCHER_LOG="${OUT_PATH}/dispatcher.log"
                WORKLOAD=${WORKLOAD} WORKER_CPUS=${NUM_CPUS} MEMORY=${MEMORY_SIZE} POWEROFF_CMD=y nohup stdbuf -oL ./launch.sh ${MODE} >"${DISPATCHER_LOG}" 2>&1 &
                wait-dispatcher "${DISPATCHER_LOG}"
                sleep 2 # for safety
                WORKLOAD=${WORKLOAD} LG_RPS=$rps LG_OUTPUT="${OUT_PATH}" ./launch.sh lr
                exp-cleanup
                ./launch.sh benchmark-upload || true
                ./launch.sh remote-benchmark-upload || true
                sleep 120 # wait for reclaim
            done
        done
    done
}

exp-repeat-hermit-fast() {
    EXP=$1
    WORKLOAD=$2
    START_KRPS=$3
    MAX_KRPS=$4
    STEP_KRPS=$5
    NUM_CPUS=$6
    MEMORY_SIZE=$7
    NUM_TRY=$8
    set +x
    if [ -z "$EXP" ]; then
        echo "no EXP"
        retrun 1
    fi
    if [ -z "$WORKLOAD" ]; then
        echo "no WORKLOAD"
        retrun 1
    fi
    if [ -z "$START_KRPS" ]; then
        echo "no START_KRPS"
        retrun 1
    fi
    if [ -z "$MAX_KRPS" ]; then
        echo "no MAX_KRPS"
        retrun 1
    fi
    if [ -z "$NUM_TRY" ]; then
        NUM_TRY=1
    fi
    if [ -z "$STEP_KRPS" ]; then
        STEP_KRPS=100
    fi
    if [ -z "$MEMORY_SIZE" ]; then
        MEMORY_SIZE=$MEMORY
    fi
    if [ -z "$NUM_CPUS" ]; then
        NUM_CPUS=1
    fi
    set -x
    exp-cleanup
    BENCHMARK_OUT_THIS="${BENCHMARK_OUT}/${EXP}/${TODAY}"
    mkdir -p "${BENCHMARK_OUT_THIS}"
    for try in $(seq 1 $NUM_TRY); do
        for MODE in ${!MODES[@]}; do

            DISPATCHER_LOG="${BENCHMARK_OUT_THIS}/dispatcher-${MODE}-${try}.log"
            WORKLOAD=${WORKLOAD} WORKER_CPUS=${NUM_CPUS} MEMORY=${MEMORY_SIZE} POWEROFF_CMD=y nohup stdbuf -oL ./launch.sh ${MODE} >"${DISPATCHER_LOG}" 2>&1 &
            wait-dispatcher "${DISPATCHER_LOG}"
            sleep 2 # for safety
            for krps in $(seq $START_KRPS $STEP_KRPS $MAX_KRPS); do
                if ((krps > ${MODES[${MODE}]})); then
                    continue
                fi
                sleep 10 # for safety
                echo "Mode: ${MODE} Exp: ${EXP} Workload: ${WORKLOAD} MEMORY_SIZE: ${MEMORY_SIZE} krps: ${krps} try: ${try}"

                OUT_PATH="${BENCHMARK_OUT_THIS}/${MODE}_${krps}krps_${try}"
                mkdir -p ${OUT_PATH}

                rps="${krps}000"
                WORKLOAD=${WORKLOAD} LG_RPS=$rps LG_OUTPUT="${OUT_PATH}" ./launch.sh lr

                ./launch.sh benchmark-upload || true
                ./launch.sh remote-benchmark-upload || true
                sleep 120 # wait for reclaim
            done
            exp-cleanup
        done
    done
}

exp-array_1() {
    MODES["rval"]=400
    MODES["rvs"]=300
    MODES["rvam"]=360
    MODES["rvhm"]=360
    MODES["rvsjk"]=360
    exp-repeat array_1 array-1 20 500 20 1
}
exp-array_8() {
    MODES["rval"]=2500
    MODES["rvs"]=2000
    # MODES["rvam"]=2000
    # MODES["rvhm"]=2000
    # MODES["rvsjk-10000"]=2000
    # MODES["rvsjk-20000"]=2000
    MODES["rvsjk2-10000"]=2000
    #MODES["rvsjk2-20000"]=2000
    exp-repeat-fast array_8 array-1 100 3500 50 8
}

exp-array-hermit_8() {
    MODES["rnh"]=1500
    exp-repeat-hermit-fast array_8 array-1 50 1500 50 8
}

exp-warray_8() {
    MODES["rval"]=2000
    #    MODES["rvs"]=1500
    # MODES["rvam"]=2000
    # MODES["rvhm"]=2000
    # MODES["rvsjk-10000"]=2000
    # MODES["rvsjk-20000"]=2000
    # MODES["rvsjk2-10000"]=1800
    #MODES["rvsjk2-20000"]=2000
    exp-repeat warray_8 warray-1 100 3500 100 8
}
exp-array_8_2() {
    MODES["rval"]=3200
    MODES["rvs"]=1800
    exp-repeat array_8_2 array-1 1000 3500 100 8
}
exp-array_8_mem() {
    MODES["rval"]=4000
    MODES["rvs"]=4000
    # MODES["rvam"]=2000
    # MODES["rvhm"]=2000
    # MODES["rvsjk-10000"]=2000
    # MODES["rvsjk-20000"]=2000
    # MODES["rvsjk2-10000"]=4000
    #MODES["rvsjk2-20000"]=2000
    exp-repeat-mem array_8_mem array-1 1000 4000 50 8
}

exp-array_cores() {
    MODES["rval"]=3200
    for core in $(seq 4 4 24); do
        exp-repeat-fast array_cores array-1 1000 6000 100 $core
    done
}

exp-array_opt() {
    MODES["rval"]=3200
    MODES["rvals1"]=3200

    exp-repeat array_opt array-1 1000 3500 100 8
}

exp-array_100() {
    MODES["rval"]=3200
    MODES["rvs"]=2000
    MODES["rvam"]=2000
    # MODES["rvhm"]=2000
    # MODES["rvsjk-10000"]=2000
    # MODES["rvsjk-20000"]=2000
    MODES["rvsjk2-10000"]=2000
    MODES["rvsjk2-20000"]=2000
    exp-repeat array_100 array-100 2 40 2 8
}

exp-array_bm() {
    MODES["rval"]=2000
    MODES["rvs"]=1000
    MODES["rvam"]=1500
    # MODES["rvsjk-10000"]=1500
    # MODES["rvsjk-20000"]=1500
    MODES["rvsjk2-10000"]=1500
    MODES["rvsjk2-20000"]=1500
    exp-repeat array_bm array-bm 100 2000 100 8
}

exp-scan_1() {
    MODES["rvs"]=700
    MODES["rval"]=1200
    MODES["rvals1"]=1200
    #MODES["rvam"]=800
    # MODES["rvhm"]=650
    # MODES["rvsjk-10000"]=650
    # MODES["rvsjk-20000"]=650
    MODES["rvsjk2-10000"]=700
    #MODES["rvsjk2-20000"]=650
    exp-repeat-fast scan_1 scan-1 100 1200 50 8
}

exp-scan-hermit_1() {

    MODES["rnh"]=700
    exp-repeat-hermit-fast scan_1 scan-1 100 1200 50 8
}

exp-scan_100() {
    MODES["rvs"]=650
    MODES["rval"]=900
    # MODES["rvam"]=650
    # MODES["rvhm"]=650
    # MODES["rvsjk-10000"]=650
    # MODES["rvsjk-20000"]=650
    MODES["rvsjk2-10000"]=650
    # MODES["rvsjk2-20000"]=650
    exp-repeat scan_100 scan-100 100 1000 100 8
}

exp-scan_bm() {
    # MODES["rvs"]=600
    # MODES["rval"]=1200
    # MODES["rvals1"]=1200
    #MODES["rvam"]=550
    # MODES["rvhm"]=550
    # MODES["rvsjk-10000"]=550
    # MODES["rvsjk-20000"]=550
    # MODES["rvsjk2-10000"]=600
    #MODES["rvsjk2-20000"]=550
    # exp-repeat-fast scan_bm scan-bm 100 1200 50 8

    do-exp \
        --fast \
        --experiment scan_bm \
        --workload scan-bm \
        --start 100k \
        --step 50k \
        --memory_size 10G \
        --mode rvals1 950k \
        --mode rval 950k
}

exp-scan-hermit_bm() {
    MODES["rnh"]=600
    exp-repeat-hermit-fast scan_bm scan-bm 100 1200 50 8
}

exp-get_1() {
    MODES["rvs"]=1000
    MODES["rval"]=1000
    # MODES["rvals1"]=1000
    #    MODES["rvam"]=1000
    MODES["rvsjk2-10000"]=1000
    #    MODES["rvsjk2-20000"]=1000
    exp-repeat-fast get_1 get-1 40 1500 20 8
}

exp-get-hermit_1() {
    MODES["rnh"]=1000
    exp-repeat-hermit-fast get_1 get-1 100 1500 20 8
}

exp-get_2() {
    MODES["rvs"]=1000
    MODES["rval"]=1000
    MODES["rvals1"]=1000
    #    MODES["rvam"]=1000
    MODES["rvsjk2-10000"]=1000
    #    MODES["rvsjk2-20000"]=1000
    exp-repeat-fast get_2 get-2 40 900 20 8
}

exp-get-hermit_2() {
    MODES["rnh"]=700
    exp-repeat-hermit-fast get_2 get-2 40 1500 40 8
}

exp-sjk() {
    MODES["rvl"]=100
    MODES["rvlsjk2-10000"]=100
    exp-repeat-local scan_sjk scan-sjk 1 50 1 8
}

exp-vsearch() {
    do-exp \
        --experiment vsearch \
        --workload vsearch \
        --start 100 \
        --step 50 \
        --memory_size 15G \
        --mode rvs 1300 \
        --mode rval 1300 \
        --mode rvsjk2-10000 1300
}

exp-vsearch-hermit() {
    do-exp \
        --native \
        --experiment vsearch \
        --workload vsearch \
        --start 100 \
        --step 50 \
        --memory_size 15G \
        --mode rnh 1300
}

exp-tpcc() {
    do-exp \
        --experiment tpcc \
        --workload tpcc \
        --start 10k \
        --step 10k \
        --memory_size 10G \
        --mode rvs 220k \
        --mode rval 220k \
        --mode rvsjk2-10000 220k

}

exp-tpcc-hermit() {
    do-exp \
        --native \
        --experiment tpcc \
        --workload tpcc \
        --start 10k \
        --step 10k \
        --memory_size 10G \
        --mode rnh 220k
}

EXPNAME=""

notify-finish() {
    HOSTNAME=$(uname -n)
    if [ -n "${SLACK_WEBHOOK}" ]; then
        curl -X POST -H 'Content-type: application/json' --data "{\"text\":\"[${HOSTNAME}] Experiment ${EXPNAME} finished!\"}" "${SLACK_WEBHOOK}"
    else
        echo "[${HOSTNAME}] Experiment ${EXPNAME} finished!"
    fi
}

experiment() {
    sync
    remote-build
    build
    trap notify-finish EXIT

    EXPNAME=${1}

    case $1 in
    *)
        "exp-${1}" || true
        ;;
    esac
}

breakdown() {
    export BREAKDOWN=1
    ./launch.sh sync
    ./launch.sh remote-build
    ./launch.sh b
    declare -a MODES=("rvs" "rvsjk2-10000" "rval")
    trap notify-finish EXIT
    BENCHMARK_OUT_THIS="${BENCHMARK_OUT}/breakdown/${TODAY}"
    for MODE in "${MODES[@]}"; do
        echo "Mode: ${MODE}"
        set -x
        OUT_PATH="${BENCHMARK_OUT_THIS}/${MODE}"
        mkdir -p ${OUT_PATH}
        exp-prepare
        DISPATCHER_LOG="${OUT_PATH}/dispatcher.log"
        WORKLOAD=array-1 WORKER_CPUS=${NUM_CPUS} POWEROFF_CMD=y nohup ./launch.sh ${MODE} >"${DISPATCHER_LOG}" 2>&1 &
        wait-dispatcher "${DISPATCHER_LOG}"
        sleep 2 # for safety
        rps="${BREAKDOWN_KRPS}000"
        WORKLOAD=array-1 LG_RPS=$rps LG_OUTPUT="${OUT_PATH}" ./launch.sh lr
        exp-cleanup
        ./launch.sh benchmark-upload || true
        ./launch.sh remote-benchmark-upload || true
    done

}
breakdown-hermit() {
    export BREAKDOWN=1
    ./launch.sh sync
    ./launch.sh remote-build
    ./launch.sh bd
    exp-cleanup
    # trap notify-finish EXIT
    MODE=rnh

    BENCHMARK_OUT_THIS="${BENCHMARK_OUT}/breakdown-hermit/${TODAY}"

    # echo "Mode: ${MODE}"
    set -x
    OUT_PATH="${BENCHMARK_OUT_THIS}/${MODE}"
    mkdir -p ${OUT_PATH}

    DISPATCHER_LOG="${OUT_PATH}/dispatcher.log"
    WORKLOAD=array-1 WORKER_CPUS=8 nohup ./launch.sh ${MODE} >"${DISPATCHER_LOG}" 2>&1 &
    wait-dispatcher "${DISPATCHER_LOG}"
    sleep 2 # for safety
    rps="300000"
    WORKLOAD=array-1 LG_RPS=$rps LG_OUTPUT="${OUT_PATH}" ./launch.sh lr
    exp-cleanup

    cp /sys/kernel/debug/hermit/spf_addrs "${OUT_PATH}/" || true
    cp /sys/kernel/debug/hermit/spf_* "${OUT_PATH}/" || true

    ./launch.sh benchmark-upload || true
    ./launch.sh remote-benchmark-upload || true

}

experiment-all() {
    ./launch.sh e array_8
    ./launch.sh e scan_1
    ./launch.sh e scan_bm
    ./launch.sh e get_1
    ./launch.sh e get_2
}

experiment-all-hermit() {
    ./launch.sh e array-hermit_8
    ./launch.sh e scan-hermit_1
    ./launch.sh e scan-hermit_bm
    ./launch.sh e get-hermit_1
    ./launch.sh e get-hermit_2
}

figure-all() {
    ./launch.sh f array_8 2024-05-15_01-07-42 2024-05-15_14-25-31
    ./launch.sh f scan_1 2024-05-15_02-39-10 2024-05-15_15-47-04
    ./launch.sh f scan_bm 2024-05-15_03-48-18 2024-05-15_16-24-59
    ./launch.sh f get_1 2024-05-15_04-33-38 2024-05-15_16-57-26
    ./launch.sh f get_2 2024-05-15_06-05-43 2024-05-15_19-04-51
}

ea2() {
    ./launch.sh e array_opt
    ./launch.sh e array_cores
    ./launch.sh e array_8
    ./launch.sh e array_bm
    ./launch.sh e scan_1
    ./launch.sh e scan_bm
}
ea3() {
    ./launch.sh e tpcc
    ./launch.sh e vsearch
}

ea3-hermit() {
    ./launch.sh e tpcc-hermit
    ./launch.sh e vsearch-hermit

}

figure() {
    SCRIPT=""

    NAME=$1
    shift
    case $NAME in
    array_cores)
        SCRIPT="script/latency-99-cores.py"
        ;;
    breakdown)
        SCRIPT="script/latency-breakdown.py"
        ;;
    breakdown-hermit)
        SCRIPT="script/latency-breakdown-hermit.py"
        ;;
    *)
        SCRIPT="script/latency-99.py"
        ;;
    esac
    if [ -f "$SCRIPT" ]; then
        mkdir -p "figure/$NAME/"
        python3 $SCRIPT $NAME "figure/$NAME/" $@
    fi
}

benchmark-upload() {
    if [ -n "${RSYNC_DEST}" ] && [ -n "${RSYNC_PORT}" ]; then
        rsync -rptgov -e "ssh -p ${RSYNC_PORT}" "${BENCHMARK_OUT}/" "${RSYNC_DEST}"
    fi

}

remote-benchmark-upload() {
    run-remote-lg "cd $ROOT_PATH; ./launch.sh benchmark-upload"

}

### --- Remote Functions END --- ###
set -x
case $1 in
install)
    install-deps
    install-ofed
    ;;
b | build)
    shift
    build $@
    ;;
bd | build-dispatcher)
    build-dispatcher
    ;;
c | clean)
    clean
    ;;
d | debug)
    debug
    ;;
t | trace)
    trace
    ;;
r | run)
    shift
    run $@
    ;;
rvh | run-vm-hybrid)
    shift
    run-vm-hybrid $@
    ;;
rva | run-vm-async)
    shift
    run-vm-async $@
    ;;
rvam | run-vm-async-mq)
    shift
    run-vm-async-mq $@
    ;;
rvhm | run-vm-hybrid-mq)
    shift
    run-vm-hybrid-mq $@
    ;;
rval | run-vm-async-local)
    shift
    run-vm-async-local $@
    ;;
rvals1 | run-vm-async-local-sched1)
    shift
    run-vm-async-local-sched1 $@
    ;;
rvs | run-vm-sync)
    shift
    run-vm-sync $@
    ;;
rvsjk | run-vm-shinjuku)
    shift
    run-vm-shinjuku $@
    ;;
rvsjk-* | run-vm-shinjuku-*)
    PARTS=(${1//-/ })
    CYCLES=${PARTS[1]}
    shift
    run-vm-shinjuku-custom $CYCLES $@
    ;;
rvsjk2-* | run-vm-shinjuku2-*)
    PARTS=(${1//-/ })
    CYCLES=${PARTS[1]}
    shift
    run-vm-shinjuku2-custom $CYCLES $@
    ;;
rvl | run-vm-local)
    shift
    run-vm-local $@
    ;;
rvlsjk2-* | run-vm-local-shinjuku2-*)
    PARTS=(${1//-/ })
    CYCLES=${PARTS[1]}
    shift
    run-vm-local-shinjuku2-custom $CYCLES $@
    ;;
rn | run-native)
    shift
    run-native $@
    ;;
rnh | run-native-hermit)
    shift
    run-native-hermit $@
    ;;
s | sync) sync ;;
kdn | kill-dispatcher-native) kill-dispatcher-native ;;
k | kill-qemu) kill-qemu ;;
sb | sync-build)
    sync
    remote-build
    ;;
rr | remote-reup)
    remote-down || true
    remote-up
    ;;
rru | remote-reupwait)
    remote-down || true
    remote-up
    wait-mem-server
    ;;
ru | remote-up)
    remote-up
    ;;
rd | remote-down)
    remote-down
    ;;
rc | remote-clean)
    remote-clean
    ;;
lg | run-loadgen)
    shift
    run-loadgen $@
    ;;
ll | loadgen-local)
    shift
    loadgen-local $@
    ;;
lr | loadgen-remote)
    shift
    loadgen-remote $@
    ;;
run-mem-server)
    shift
    run-mem-server $@
    ;;
e | exp | experiment)
    shift
    experiment $@
    ;;
ea | experiment-all)
    experiment-all
    ;;
eah | experiment-all-hermit)
    experiment-all-hermit
    ;;
f | figure)
    shift
    figure $@
    ;;
do-exp)
    shift
    do-exp $@
    ;;
benchmark-upload)
    benchmark-upload
    ;;
*)
    $1 $2
    ;;
esac
