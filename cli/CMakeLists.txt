project(ANGORTCLI)
find_package(Readline REQUIRED)

include_directories(${ANGORT_SOURCE_DIR}/include ${READLINE_INCLUDE_DIR})
include_directories(${ANGORT_BINARY_DIR}/lib)

set(SOURCE main.cpp)

add_executable(angortcli ${SOURCE})

# we change the name of angortcli to angort, so it appears under that
# latter name in the build directory

set_target_properties(angortcli PROPERTIES
    OUTPUT_NAME angort)

target_link_libraries(angortcli angort ${READLINE_LIBRARY})

install(TARGETS angortcli
    RUNTIME DESTINATION bin)
    
    
