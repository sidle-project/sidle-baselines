file (GLOB pactree_SRCS "*.h" "*.cpp")
add_library(pactree STATIC
    ${pactree_SRCS}
)
target_include_directories(pactree PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
add_library(pactree2 STATIC
    ${pactree_SRCS}
)
target_include_directories(pactree2 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_definitions(pactree2 PUBLIC
    READ_LATEST
)
add_executable (main main.cpp)
target_link_libraries (main pactree numa jemalloc pdlart pmemobj pmem)
INSTALL(TARGETS pactree 
        ARCHIVE DESTINATION ${CMAKE_SOURCE_DIR}
        )
