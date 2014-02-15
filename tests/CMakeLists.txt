project(ANGORTTEST)

include_directories(${ANGORT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${ANGORT_BINARY_DIR}/lib)

set(ANGORTDIR ../..)
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
            COMMAND perl ${ANGORTDIR}/makeWords.pl ${CMAKE_CURRENT_SOURCE_DIR}/${file} >${file}.cpp
        )
        set(WORDFILELIST ${WORDFILELIST} ${file}.cpp)
    endforeach(file)
endmacro(add_words_files)


add_words_files(wordsTest.cpp)

set(SOURCES main.cpp null.cpp hash.cpp
    ${WORDFILELIST})

add_executable(tests ${SOURCES})

target_link_libraries(tests angort)
