cmake_minimum_required(VERSION 2.6)

project(ANGORTLIB)


IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_definitions(-DLINUX)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    
add_compile_options("-Woverloaded-virtual")

# would be awesome if these could live in an angort cmake module

set(WORDFILELIST)
# this macro generates a custom build step which takes the files passed in,
# passes them through Angort's 'makeWords.pl' script, and compiles *those*.
# The files generated are added to WORDFILELIST, which should then be put
# into the SOURCES. So, Angort word files should be added with this.

# a problem with this approach is when make -j is used: all the words
# files are called words.tex, so they overwrite each other. The
# makeWords.pl script needs modification (each run producing a differently
# named file) to deal with this but that would break existing build scripts.

# Another problem is that this only works when done from a completely
# empty build directory - even "make clean" isn't sufficient! No idea why.

macro(add_words_files)
    add_custom_command(
        OUTPUT wordslist allwords.tex
        COMMAND rm wordslist
        COMMAND rm allwords.tex
        )
    foreach(file ${ARGV})
        add_custom_command(
            OUTPUT ${file}.cpp
            DEPENDS ${file} ${CMAKE_SOURCE_DIR}/makeWords.pl
            COMMAND pwd
            COMMAND perl  ../../makeWords.pl  ${CMAKE_SOURCE_DIR}/lib/${file}>${file}.cpp
            COMMAND cat words >>wordslist
            COMMAND cat words.tex >>allwords.tex
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
    
add_definitions(-Wall)
#add_definitions(-g)

add_words_files(stdWords.cpp   stdColl.cpp  stdString.cpp  stdMath.cpp
stdEnv.cpp future.cpp deprecated.cpp)

set(SOURCE angort.cpp tokeniser.cpp tokens.cpp types.cpp namespace.cpp
    cycle.cpp   binop.cpp   plugins.cpp    format.cpp    stringbuf.cpp
    filefind.cpp value.cpp
    types/closure.cpp    types/int.cpp    types/float.cpp     types/string.cpp
    types/range.cpp      types/code.cpp   types/iter.cpp      types/list.cpp
    types/hashtype.cpp         types/symbol.cpp       types/native.cpp
    types/long.cpp types/double.cpp types/nsid.cpp
    ${WORDFILELIST})

add_library(angort ${SOURCE})

if(PROFILE)
    message("profiling enabled")
    target_compile_options(angort PUBLIC "-pg")
endif()

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(angort dl m)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")


install(TARGETS angort
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )
   
install(DIRECTORY
    ${ANGORT_SOURCE_DIR}/include/
    DESTINATION "include/angort"
    FILES_MATCHING PATTERN "*.h"
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
    
    
