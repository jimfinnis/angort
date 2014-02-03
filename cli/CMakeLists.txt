project(ANGORTCLI)
find_package(Readline REQUIRED)

include_directories(${ANGORT_SOURCE_DIR}/include ${READLINE_INCLUDE_DIR})
include_directories(${ANGORT_BINARY_DIR}/lib)

set(WORDFILELIST)
# this macro generates a custom build step which takes the files passed in,
# passes them through Angort's 'makeWords.pl' script, and compiles *those*.
# The files generated are added to WORDFILELIST, which should then be put
# into the SOURCES. So, Angort word files should be added with this.

macro(add_words_files)
    foreach(file ${ARGV})
        add_custom_command(
            OUTPUT ${file}.cpp
            DEPENDS ${file}
            COMMAND pwd
            COMMAND perl ../../makeWords.pl ${CMAKE_SOURCE_DIR}/cli/${file} >${file}.cpp
        )
        set(WORDFILELIST ${WORDFILELIST} ${file}.cpp)
    endforeach(file)
endmacro(add_words_files)

add_words_files(stdMath.cpp)

set(SOURCE main.cpp ${WORDFILELIST})

add_executable(angortcli ${SOURCE})
target_link_libraries(angortcli angort ${READLINE_LIBRARY})
