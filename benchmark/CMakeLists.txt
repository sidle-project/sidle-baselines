cmake_minimum_required (VERSION 3.5)
project (SIDLE-benchmark)

###############################################################################
# read_latest_origin
###############################################################################
add_executable(read_latest_origin
    ${CMAKE_CURRENT_SOURCE_DIR}/../benchmark/micro/read_latest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/helper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/art/art.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/art/art_node.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/wormhole/wh.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/cxl_utils/cxl_allocator.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/wormhole/wh.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/wormhole/kv.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/wormhole/lib.c
)
target_compile_definitions(read_latest_origin
    PRIVATE
        ${NDEBUG}
        ${NDEBUGGING}
        ${CORRECTNESS}
        ORIGIN_YCSB
        TF_GRAPH_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../asset"
)
target_link_libraries(read_latest_origin
    PRIVATE
        -lpthread
        LIBJSON
        memkind
)
target_link_directories(read_latest_origin PRIVATE /usr/local/lib)


###############################################################################
# short_range_origin
###############################################################################
add_executable(short_range_origin
    ${CMAKE_CURRENT_SOURCE_DIR}/../benchmark/micro/short_range.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/helper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/art/art.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/art/art_node.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/wormhole/wh.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/cxl_utils/cxl_allocator.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/wormhole/wh.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/wormhole/kv.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/wormhole/lib.c
)
target_compile_definitions(short_range_origin
    PRIVATE
        ${NDEBUG}
        ${NDEBUGGING}
        ${CORRECTNESS}
        ORIGIN_YCSB
        TF_GRAPH_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../asset"
)
target_link_libraries(short_range_origin
    PRIVATE
        -lpthread
        LIBJSON
        memkind
)
target_link_directories(short_range_origin PRIVATE /usr/local/lib)

###############################################################################
# rw_ycsb_origin
###############################################################################
add_executable(rw_ycsb_origin
    ${CMAKE_CURRENT_SOURCE_DIR}/../benchmark/micro/rw_ycsb.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/helper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/art/art.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/art/art_node.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/wormhole/wh.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/cxl_utils/cxl_allocator.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/wormhole/wh.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/wormhole/kv.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/wormhole/lib.c
)
target_compile_definitions(rw_ycsb_origin
    PRIVATE
        ${NDEBUG}
        ${NDEBUGGING}
        ${CORRECTNESS}
        ORIGIN_YCSB
        TF_GRAPH_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../asset"
)
target_link_libraries(rw_ycsb_origin
    PRIVATE
        -lpthread
        LIBJSON
        memkind
)
target_link_directories(rw_ycsb_origin PRIVATE /usr/local/lib)

###############################################################################
# read_modify_write_origin
###############################################################################
add_executable(read_modify_write_origin
    ${CMAKE_CURRENT_SOURCE_DIR}/../benchmark/micro/read_modify_write.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/helper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/art/art.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/art/art_node.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/wormhole/wh.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/cxl_utils/cxl_allocator.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/wormhole/wh.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/wormhole/kv.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/wormhole/lib.c
)
target_compile_definitions(read_modify_write_origin
    PRIVATE
        ${NDEBUG}
        ${NDEBUGGING}
        ${CORRECTNESS}
        ORIGIN_YCSB
        TF_GRAPH_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../asset"
)
target_link_libraries(read_modify_write_origin
    PRIVATE
        -lpthread
        LIBJSON
        memkind
)
target_link_directories(read_modify_write_origin PRIVATE /usr/local/lib)


###############################################################################
# skewed_partition
###############################################################################
add_executable(skewed_partition
    ${CMAKE_CURRENT_SOURCE_DIR}/../benchmark/micro/skewed_partition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/helper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/art/art.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/art/art_node.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/wormhole/wh.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/cxl_utils/cxl_allocator.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/wormhole/kv.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/wormhole/lib.c
)
target_compile_definitions(skewed_partition
    PRIVATE
        ${NDEBUG}
        ${NDEBUGGING}
        ${CORRECTNESS}
        NO_DYNAMIC
        # ORDERED
        TF_GRAPH_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../asset"
)
target_link_libraries(skewed_partition
    PRIVATE
        -lpthread
        LIBJSON
        memkind
)
target_link_directories(skewed_partition PRIVATE /usr/local/lib)


###############################################################################
# skewed_parition_dynamic
###############################################################################
add_executable(skewed_partition_dynamic 
    ${CMAKE_CURRENT_SOURCE_DIR}/../benchmark/micro/skewed_partition_dynamic.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/helper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/art/art.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/art/art_node.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/wormhole/wh.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/cxl_utils/cxl_allocator.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/wormhole/kv.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/wormhole/lib.c
)
target_compile_definitions(skewed_partition_dynamic 
    PRIVATE
        ${NDEBUG}
        ${NDEBUGGING}
        ${CORRECTNESS}
        # ORDERED
)
target_link_libraries(skewed_partition_dynamic 
    PRIVATE
        -lpthread
        LIBJSON
        memkind
)
target_link_directories(skewed_partition_dynamic PRIVATE /usr/local/lib)

###############################################################################
# real world trace
###############################################################################
add_executable(real_world_trace
    ${CMAKE_CURRENT_SOURCE_DIR}/../benchmark/micro/real_world_trace.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/helper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/art/art.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/art/art_node.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/cxl_utils/cxl_allocator.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/wormhole/wh.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/wormhole/kv.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/wormhole/lib.c
)
target_compile_definitions(real_world_trace
    PRIVATE
        ${NDEBUG}
        ${NDEBUGGING}
        ${CORRECTNESS}
        USE_MT
        REAL_WORLD
	    ANALYZE_LATENCY
        TF_GRAPH_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../asset"
)
target_link_libraries(real_world_trace
    PRIVATE
        -lpthread
        LIBJSON
        memkind
)
target_link_directories(real_world_trace PRIVATE /usr/local/lib)

###############################################################################
# launch_bench_nopid
###############################################################################
add_executable(launch_bench_nopid
    ${CMAKE_CURRENT_SOURCE_DIR}/../benchmark/memtis_utils/launch_bench.c
)

target_compile_definitions(launch_bench_nopid
    PRIVATE
    __NOPID
)

###############################################################################
# launch_bench
###############################################################################
add_executable(launch_bench
    ${CMAKE_CURRENT_SOURCE_DIR}/../benchmark/memtis_utils/launch_bench.c
)

###############################################################################
# kill_ksampled
###############################################################################
add_executable(kill_ksampled
    ${CMAKE_CURRENT_SOURCE_DIR}/../benchmark/memtis_utils/kill_ksampled.c
)

