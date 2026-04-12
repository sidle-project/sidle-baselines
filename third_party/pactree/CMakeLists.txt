cmake_minimum_required(VERSION 3.5)

# set(CMAKE_C_COMPILER "clang")
# set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_BUILD_TYPE Release)

project(sidle-pactree)

execute_process(COMMAND cat /proc/cpuinfo COMMAND grep clwb OUTPUT_VARIABLE CLWB)
if(CLWB)
    message(STATUS "Use clwb")
else()
        add_definitions(-DNO_CLWB)
    message(STATUS "Use clflush instaed of clwb")
endif()

# set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
# set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-pthread -Wall -Wextra -march=native -mavx512bw -lmemkind")
# set(CMAKE_CXX_FLAGS "-pthread -Wall -Wextra -march=native -lmemkind -O0 -g")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
enable_testing()

add_subdirectory(lib/PDL-ART)
add_subdirectory(src)
#add_subdirectory(tests)
#add_subdirectory(benchmarks)
