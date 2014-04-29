cmake_minimum_required(VERSION 2.6)



project(ANGORTLIB)

# would be awesome if these could live in an angort cmake module

set(WORDFILELIST)
# this macro generates a custom build step which takes the files passed in,
# passes them through Angort's 'makeWords.pl' script, and compiles *those*.
# The files generated are added to WORDFILELIST, which should then be put
# into the SOURCES. So, Angort word files should be added with this.

macro(add_words_files)
    add_custom_command(
        OUTPUT wordslist
        COMMAND rm wordslist
        )
    foreach(file ${ARGV})
        add_custom_command(
            OUTPUT ${file}.cpp
            DEPENDS ${file}
            COMMAND pwd
            COMMAND perl  ../../makeWords.pl  ${CMAKE_SOURCE_DIR}/lib/${file}>${file}.cpp
            COMMAND cat words >>wordslist
        )
        set(WORDFILELIST ${WORDFILELIST} ${file}.cpp)
    endforeach(file)
endmacro(add_words_files)

# this command uses a python script to build the tokens files

add_custom_command(
    OUTPUT tokens.cpp tokens.h
    DEPENDS tokens
    COMMAND python ${CMAKE_SOURCE_DIR}/lib/gentoks ${CMAKE_SOURCE_DIR}/lib/tokens tokens)

include_directories(${ANGORT_SOURCE_DIR}/include ${ANGORT_SOURCE_DIR}/lib
    ${ANGORT_SOURCE_DIR}/build/lib)
    
#add_definitions(-g)

add_words_files(stdWords.cpp stdLists.cpp)

set(SOURCE angort.cpp tokeniser.cpp tokens.cpp types.cpp ser.cpp namespace.cpp
    cycle.cpp
    types/closure.cpp    types/int.cpp    types/float.cpp     types/string.cpp
    types/range.cpp      types/code.cpp      types/iter.cpp     types/list.cpp
    types/hashtype.cpp   types/symbol.cpp
    ${WORDFILELIST})

add_library(angort ${SOURCE})


