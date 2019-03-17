cmake_minimum_required(VERSION 2.6)
project(ANGORT)
#variable_watch(ANGORT_SOURCE_DIR)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

option(PROFILE "enable profiling")
option(EDITLINE_NOUNICODE "using a version of editline with no unicode support")
option(POSIXTHREADS "enable POSIX threading (experimental")
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    message("Linux detected")
#    set(POSIXTHREADS 1)
ELSE()
#    set(POSIXTHREADS 0)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

# build a config file into the include dir so other projects know
# how Angort was built.

message("Writing config")
configure_file(${ANGORT_SOURCE_DIR}/config.h.in ${ANGORT_SOURCE_DIR}/include/config.h)



link_directories(${ANGORT_SOURCE_DIR}/lib)

add_subdirectory(cli)
add_subdirectory(tests)
add_subdirectory(lib)

enable_testing()

# these tests are for C++ linkage stuff; there's a simple
# test suite manager in there

add_test(cpptests tests/tests ${ANGORT_SOURCE_DIR}/tests)

# these tests are for plain angort

add_test(basic cli/angort ${ANGORT_SOURCE_DIR}/testfiles/basic.ang)
add_test(strings cli/angort ${ANGORT_SOURCE_DIR}/testfiles/strings.ang)
add_test(cond cli/angort ${ANGORT_SOURCE_DIR}/testfiles/cond.ang)
add_test(loop cli/angort ${ANGORT_SOURCE_DIR}/testfiles/loop.ang)

add_test(closure cli/angort ${ANGORT_SOURCE_DIR}/testfiles/closure.ang)
add_test(recurse cli/angort ${ANGORT_SOURCE_DIR}/testfiles/recurse.ang)
add_test(range cli/angort ${ANGORT_SOURCE_DIR}/testfiles/range.ang)
add_test(list cli/angort ${ANGORT_SOURCE_DIR}/testfiles/list.ang)
add_test(hash cli/angort ${ANGORT_SOURCE_DIR}/testfiles/hash.ang)
add_test(cycle cli/angort ${ANGORT_SOURCE_DIR}/testfiles/cycle.ang)
add_test(constexpr cli/angort ${ANGORT_SOURCE_DIR}/testfiles/constexpr.ang)
add_test(exception cli/angort ${ANGORT_SOURCE_DIR}/testfiles/exception.ang)
add_test(incdec cli/angort ${ANGORT_SOURCE_DIR}/testfiles/incdec.ang)
add_test(generator cli/angort ${ANGORT_SOURCE_DIR}/testfiles/generator.ang)

add_test(format cli/angort ${ANGORT_SOURCE_DIR}/testfiles/format.ang)

# this only works in the testfiles directory.
#add_test(pkg cli/angort ${ANGORT_SOURCE_DIR}/testfiles/pkg.ang)

file(GLOB anglibfiles "${CMAKE_SOURCE_DIR}/anglibs/*.ang")
foreach(item ${anglibfiles})
    if(${item} MATCHES "${CMAKE_SOURCE_DIR}/anglibs/test*")
        list(REMOVE_ITEM anglibfiles ${item})
    endif(${item} MATCHES "${CMAKE_SOURCE_DIR}/anglibs/test*")
endforeach(item)

install(FILES ${anglibfiles} makeWords.pl DESTINATION share/angort)



