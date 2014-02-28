cmake_minimum_required(VERSION 2.6)
project(ANGORT)
#variable_watch(ANGORT_SOURCE_DIR)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

link_directories(${ANGORT_SOURCE_DIR}/lib)

add_definitions(-g)

add_subdirectory(cli)
add_subdirectory(tests)
add_subdirectory(lib)

enable_testing()

# these tests are for C++ linkage stuff; there's a simple
# test suite manager in there

add_test(cpptests tests/tests ${ANGORT_SOURCE_DIR}/tests)

# these tests are for plain angort

add_test(basic cli/angortcli ${ANGORT_SOURCE_DIR}/testfiles/basic.ang)
add_test(strings cli/angortcli ${ANGORT_SOURCE_DIR}/testfiles/strings.ang)
add_test(cond cli/angortcli ${ANGORT_SOURCE_DIR}/testfiles/cond.ang)
add_test(loop cli/angortcli ${ANGORT_SOURCE_DIR}/testfiles/loop.ang)
add_test(closure cli/angortcli ${ANGORT_SOURCE_DIR}/testfiles/closure.ang)
add_test(recurse cli/angortcli ${ANGORT_SOURCE_DIR}/testfiles/recurse.ang)
add_test(range cli/angortcli ${ANGORT_SOURCE_DIR}/testfiles/range.ang)
add_test(saveload cli/angortcli ${ANGORT_SOURCE_DIR}/testfiles/saveload.ang)
add_test(list cli/angortcli ${ANGORT_SOURCE_DIR}/testfiles/list.ang)
