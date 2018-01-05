project(ANGORTTEST)

include_directories(${ANGORT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${ANGORT_BINARY_DIR}/lib)

set(ANGORTDIR ../..)

set(SOURCES main.cpp null.cpp hash.cpp
    ${WORDFILELIST})

add_executable(tests ${SOURCES})

target_link_libraries(tests angort pthread)
