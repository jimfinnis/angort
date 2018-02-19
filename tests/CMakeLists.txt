project(ANGORTTEST)

include_directories(${ANGORT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${ANGORT_BINARY_DIR}/lib)

if(POSIXTHREADS)
    message("POSIX threads enabled")
    set(PTHREAD "-lpthread")
else()  
    set(PTHREAD "")
endif(POSIXTHREADS)

set(ANGORTDIR ../..)

set(SOURCES main.cpp null.cpp hash.cpp
    ${WORDFILELIST})

add_executable(tests ${SOURCES})

if(POSIXTHREADS)
    set_target_properties(tests PROPERTIES COMPILE_DEFINITIONS ANGORT_POSIXLOCKS)
    target_compile_options(tests PUBLIC "-pthread")
endif(POSIXTHREADS)

target_link_libraries(tests angort pthread)
