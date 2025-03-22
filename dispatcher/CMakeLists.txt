cmake_minimum_required(VERSION 3.13)
project(
  dispatcher
  VERSION 1.0.0
  LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -Wformat -Wunused-variable -Werror=format -Werror=return-type -fno-stack-protector"
)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-stack-protector")

option(PROTOCOL_MEASURE "" ON)
option(PROTOCOL_BREAKDOWN "" OFF)
option(ENABLE_SILO "" OFF)
option(ENABLE_FAISS "" OFF)

set(DRIVER_LINUX_SRCS src/linux.cc src/linux-mmap.cc)
set(PROTOCOL_INCLUDES include/protocol/cmd.hh include/protocol/net.hh
                      include/protocol/req.hh)
set(DISPATCHER_INCLUDES
    include/dispatcher/common.hh
    include/dispatcher/dispatcher.hh
    include/dispatcher/job.hh
    include/dispatcher/logger.hh
    include/dispatcher/mm.hh
    include/dispatcher/worker.hh
    include/dispatcher/net.hh
    ${PROTOCOL_INCLUDES})
set(DISPATCHER_SRCS
    src/app.cc
    src/context-asm.S
    src/context.cc
    src/ucontext.cc
    src/dispatcher.cc
    src/worker.cc
    src/mm.cc
    src/job.cc
    src/rdma.cc
    src/raw-eth.cc
    src/main.cc
    src/nanocontext.S
    src/nanocontext-mk.c
    lib/rocksdb.cc
    lib/memcached.c)

if(ENABLE_SILO)
set(DISPATCHER_SRCS ${DISPATCHER_SRCS} lib/silo.cc)
endif()

if(ENABLE_FAISS)
set(DISPATCHER_SRCS ${DISPATCHER_SRCS} lib/faiss.cc)
endif()

include_directories(include)
include_directories(../dilos/include)

find_library(VERBS ibverbs REQUIRED)
find_package(Threads REQUIRED)

configure_file(include/config/cmake-config.hh.in include/config/cmake-config.hh
               @ONLY)

include_directories("${CMAKE_CURRENT_BINARY_DIR}/include")


add_library(dispatcher_linux_lib ${DISPATCHER_SRCS} ${DISPATCHER_INCLUDES}
                                 ${DRIVER_LINUX_SRCS})

target_link_libraries(dispatcher_linux_lib PRIVATE ${VERBS})
target_link_libraries(dispatcher_linux_lib PRIVATE Threads::Threads dl)
target_compile_definitions(dispatcher_linux_lib
                           PRIVATE DRIVER_LINUX HAVE_CONFIG_H DISABLE_MAP_DDC)

add_library(dispatcher_osv ${DISPATCHER_SRCS} ${DISPATCHER_INCLUDES}
                           ${DRIVER_OSV_SRCS})
target_link_libraries(dispatcher_osv PRIVATE Threads::Threads dl)
target_compile_definitions(dispatcher_osv PRIVATE DRIVER_OSV HAVE_CONFIG_H)

add_executable(dispatcher_linux bin/linux.cc)
target_link_libraries(dispatcher_linux PRIVATE dispatcher_linux_lib)

add_executable(loadgen bin/loadgen.cc src/raw-eth.cc)
target_link_libraries(loadgen PRIVATE ${VERBS})
target_link_libraries(loadgen PRIVATE Threads::Threads)

add_executable(mem-server bin/mem-server.cc src/rdma.cc)
target_link_libraries(mem-server PRIVATE ${VERBS})

add_executable(test-ctx bin/test-ctx.cc src/context-asm.S src/ucontext.cc
                        src/nanocontext.S src/nanocontext-mk.c)
target_link_libraries(test-ctx PRIVATE Threads::Threads)

# ROCKS DB ###

option(ROCKSDB_BUILD_SHARED "" OFF)
option(WITH_JNI "build with JNI" OFF)
option(WITH_TESTS "" OFF)
option(WITH_BENCHMARK_TOOLS "build with benchmarks" OFF)
option(WITH_CORE_TOOLS "build with ldb and sst_dump" OFF)
option(WITH_TOOLS "build with tools" OFF)
option(WITH_ALL_TESTS "" OFF)
option(WITH_TRACE_TOOLS "" OFF)

add_subdirectory(deps/rocksdb)
# ROCKS DB Link ###

include_directories(${rocksdb_SOURCE_DIR}/include)

target_link_libraries(dispatcher_linux_lib PRIVATE rocksdb)
target_link_libraries(dispatcher_osv PRIVATE rocksdb)


add_executable(rocksdb-test bin/rocksdb_test.cc lib/rocksdb.cc
                            src/linux-mmap.cc)
target_link_libraries(rocksdb-test PRIVATE rocksdb dl)

add_executable(rocksdb-create bin/rocksdb_create.cc)
target_link_libraries(rocksdb-create PRIVATE rocksdb)

set(MEMCACHED_SRC
    deps/memcached/memcached.c
    deps/memcached/memcached.h
    deps/memcached/hash.c
    deps/memcached/hash.h
    deps/memcached/jenkins_hash.c
    deps/memcached/jenkins_hash.h
    deps/memcached/murmur3_hash.c
    deps/memcached/murmur3_hash.h
    deps/memcached/queue.h
    deps/memcached/slabs.c
    deps/memcached/slabs.h
    deps/memcached/items.c
    deps/memcached/items.h
    deps/memcached/assoc.c
    deps/memcached/assoc.h
    deps/memcached/thread.c
    deps/memcached/daemon.c
    deps/memcached/stats_prefix.c
    deps/memcached/stats_prefix.h
    deps/memcached/util.c
    deps/memcached/util.h
    deps/memcached/trace.h
    deps/memcached/cache.c
    deps/memcached/cache.h
    deps/memcached/sasl_defs.h
    deps/memcached/bipbuffer.c
    deps/memcached/bipbuffer.h
    deps/memcached/base64.c
    deps/memcached/base64.h
    deps/memcached/logger.c
    deps/memcached/logger.h
    deps/memcached/crawler.c
    deps/memcached/crawler.h
    deps/memcached/itoa_ljust.c
    deps/memcached/itoa_ljust.h
    deps/memcached/slab_automove.c
    deps/memcached/slab_automove.h
    deps/memcached/authfile.c
    deps/memcached/authfile.h
    deps/memcached/restart.c
    deps/memcached/restart.h
    deps/memcached/proto_text.c
    deps/memcached/proto_text.h
    deps/memcached/proto_bin.c
    deps/memcached/proto_bin.h)

add_library(memcached STATIC ${MEMCACHED_SRC})
target_compile_definitions(memcached PRIVATE HAVE_CONFIG_H)

target_link_libraries(dispatcher_linux_lib PRIVATE memcached)
target_link_libraries(dispatcher_osv PRIVATE memcached)

add_executable(memcached-test bin/memcached_test.cc lib/memcached.c)
target_link_libraries(memcached-test PRIVATE memcached)
target_compile_definitions(memcached-test PRIVATE HAVE_CONFIG_H)
target_link_libraries(memcached-test PRIVATE Threads::Threads dl)


if(ENABLE_FAISS)
option(FAISS_ENABLE_GPU "" OFF)
option(FAISS_ENABLE_PYTHON "" OFF)
option(FAISS_ENABLE_C_API "" OFF)
option(FAISS_OPT_LEVEL "" "generic")
add_subdirectory(deps/faiss)

add_executable(faiss-rm-hdr bin/faiss_rm_hdr.cc)
add_executable(faiss-slice bin/faiss_slice.cc)
add_executable(faiss-create bin/faiss_create.cc)
target_link_libraries(faiss-create PRIVATE faiss_avx2)
add_executable(faiss-search bin/faiss_search.cc src/linux-mmap.cc)
target_link_libraries(faiss-search PRIVATE faiss_avx2 dl)

target_link_libraries(dispatcher_linux_lib PRIVATE faiss)
target_link_libraries(dispatcher_osv PRIVATE faiss)

endif()


if(ENABLE_SILO)
add_subdirectory(deps/silo)

include_directories(deps/silo/silo)
target_link_libraries(dispatcher_linux_lib PRIVATE silo)
target_link_libraries(dispatcher_osv PRIVATE silo)


add_executable(silo-test bin/silo_test.cc  src/linux-mmap.cc)
target_link_libraries(silo-test PRIVATE silo)
target_link_libraries(silo-test PRIVATE Threads::Threads dl)

endif()