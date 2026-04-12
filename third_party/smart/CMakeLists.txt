cmake_minimum_required (VERSION 3.5)
project (sidle-smart)

# compiler options for smart according to original smart project
set(CMAKE_C_FLAGS "-Wall -Wno-deprecated-declarations -Wno-unused-variable -Wno-unused-but-set-variable -Wsign-compare -O3")  # -O3
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++17")

set(SMART_LINKS_FLAGS "-lcityhash -lboost_system -lboost_coroutine -lpthread -libverbs -lmemcached -ltbb -lmemkind")

# for fedora
set(CMAKE_LIBRARY_PATH "${CMAKE_LIBRARY_PATH} /usr/local/lib/")
find_library(CITYHASH_LIB cityhash PATHS /usr/local/lib/libcityhash.so)

# compiler options for smart according to original smart project
option(STATIC_MN_IP "Enable static MN IP" ON)
option(ENABLE_CACHE "Enable caching" ON)
option(LONG_TEST_EPOCH "Enable long test epoch" OFF)
option(SHORT_TEST_EPOCH "Enable short test epoch" OFF)
option(MIDDLE_TEST_EPOCH "Enable middle test epoch" OFF)
option(ART_INDEXED_CACHE "Enable ART indexed cache" ON)
option(HOMOGENEOUS_INTERNAL_NODE "Enable homogeneous internal node" ON)
option(LOCK_FREE_INTERNAL_NODE "Enable lock-free internal node" ON)
option(UPDATE_IN_PLACE_LEAF_NODE "Enable update in place leaf node" ON)
option(REAR_EMBEDDED_LOCK "Enable rear embedded lock" ON)
option(READ_DELEGATION "Enable read delegation" ON)
option(WRITE_COMBINING "Enable write combining" ON)

add_definitions(-DTREE_ENABLE_ART)

if(STATIC_MN_IP)
    add_definitions(-DSTATIC_ID_FROM_IP)
else()
    remove_definitions(-DSTATIC_ID_FROM_IP)
endif()

if(ENABLE_CORO)
    add_definitions(-DUSE_CORO)
else()
    remove_definitions(-DUSE_CORO)
endif()


if(ENABLE_CACHE)
    add_definitions(-DTREE_ENABLE_CACHE)
else()
    remove_definitions(-DTREE_ENABLE_CACHE)
endif()

if(LONG_TEST_EPOCH)
    add_definitions(-DLONG_TEST_EPOCH)
else()
    remove_definitions(-DLONG_TEST_EPOCH)
endif()

if(SHORT_TEST_EPOCH)
    add_definitions(-DSHORT_TEST_EPOCH)
else()
    remove_definitions(-DSHORT_TEST_EPOCH)
endif()

if(MIDDLE_TEST_EPOCH)
    add_definitions(-DMIDDLE_TEST_EPOCH)
else()
    remove_definitions(-DMIDDLE_TEST_EPOCH)
endif()

##Cache
if(ART_INDEXED_CACHE)
    add_definitions(-DCACHE_ENABLE_ART)
else()
    remove_definitions(-DCACHE_ENABLE_ART)
endif()

##Tree
if(HOMOGENEOUS_INTERNAL_NODE)
    add_definitions(-DTREE_ENABLE_FINE_GRAIN_NODE)
else()
    remove_definitions(-DTREE_ENABLE_FINE_GRAIN_NODE)
endif()

if(LOCK_FREE_INTERNAL_NODE)
    remove_definitions(-DTREE_TEST_ROWEX_ART)
else()
    add_definitions(-DTREE_TEST_ROWEX_ART)
endif()

if(UPDATE_IN_PLACE_LEAF_NODE)
    add_definitions(-DTREE_ENABLE_IN_PLACE_UPDATE)
else()
    remove_definitions(-DTREE_ENABLE_IN_PLACE_UPDATE)
endif()

if(REAR_EMBEDDED_LOCK)
    add_definitions(-DTREE_ENABLE_EMBEDDING_LOCK)
else()
    remove_definitions(-DTREE_ENABLE_EMBEDDING_LOCK)
endif()

if(READ_DELEGATION)
    add_definitions(-DTREE_ENABLE_READ_DELEGATION)
else()
    remove_definitions(-DTREE_ENABLE_READ_DELEGATION)
endif()

if(WRITE_COMBINING)
    remove_definitions(-DTREE_TEST_HOCL_HANDOVER)
    add_definitions(-DTREE_ENABLE_WRITE_COMBINING)
else()
    add_definitions(-DTREE_TEST_HOCL_HANDOVER)
    remove_definitions(-DTREE_ENABLE_WRITE_COMBINING)
endif()

# Include files
set(INCLUDE_BASE ${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${INCLUDE_BASE})
# Source files
set(COMMON_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)

file(GLOB_RECURSE COMMON_FILE ${COMMON_SRC}/*.cpp)
add_library(SMART STATIC ${COMMON_FILE})
target_link_libraries(SMART ${SMART_LINKS_FLAGS})