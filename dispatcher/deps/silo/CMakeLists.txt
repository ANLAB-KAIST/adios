

execute_process(COMMAND ./configure --enable-max-key-len=1024 --disable-assertions --disable-invariants --disable-preconditions --with-malloc=malloc
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/silo/masstree
  COMMAND_ERROR_IS_FATAL ANY)

add_library(silo
STATIC
silo/allocator.cc
silo/btree.cc
silo/core.cc
silo/counter.cc
silo/memory.cc
silo/rcu.cc
silo/stats_server.cc
silo/thread.cc
silo/ticker.cc
silo/tuple.cc
silo/txn_btree.cc
silo/txn.cc
silo/txn_proto2_impl.cc
silo/varint.cc
silo/masstree/compiler.cc
silo/masstree/str.cc
silo/masstree/string.cc
silo/masstree/straccum.cc
silo/masstree/json.cc
silo/benchmarks/bdb_wrapper.cc
silo/benchmarks/bench.cc
silo/benchmarks/encstress.cc
silo/benchmarks/bid.cc
silo/benchmarks/masstree/kvrandom.cc
silo/benchmarks/queue.cc
silo/benchmarks/tpcc.cc
silo/benchmarks/ycsb.cc
)

message("CMAKE_CURRENT_LIST_DIR is ${CMAKE_CURRENT_LIST_DIR}")  
target_compile_definitions(silo PRIVATE -DCONFIG_H="config/config-perf.h")
target_compile_definitions(silo PRIVATE -DNDB_MASSTREE)
target_compile_options(silo PRIVATE -include ${CMAKE_CURRENT_LIST_DIR}/silo/masstree/config.h)