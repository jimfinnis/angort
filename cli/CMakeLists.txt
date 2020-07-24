project(ANGORTCLI)


include_directories(${ANGORT_BINARY_DIR}/lib ${ANGORT_BINARY_DIR}/cli)

if(POSIXTHREADS)
    message("CLI POSIX threads enabled")
    set(PTHREAD "-lpthread")
else()  
    set(PTHREAD "")
endif(POSIXTHREADS)

set(INCDIRS "${ANGORT_SOURCE_DIR}/include")

if(NOT NOLINEEDITING)
    find_package(Libedit REQUIRED)
    set(INCDIRS ${INCDIRS} ${LIBEDIT_INCLUDE_DIR})
endif(NOT NOLINEEDITING)

message(${INCDIRS})
include_directories(Includes: ${INCDIRS})

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
            COMMAND perl  ../../makeWords.pl ${CMAKE_SOURCE_DIR}/cli/${file}>${file}.cpp
            COMMAND cat words >>wordslist
        )
        set(WORDFILELIST ${WORDFILELIST} ${file}.cpp)
    endforeach(file)
endmacro(add_words_files)


add_words_files(cliWords.cpp)

add_custom_command(
    OUTPUT debtoks.cpp debtoks.h
    DEPENDS debtoks
    COMMAND python3 ${CMAKE_SOURCE_DIR}/lib/gentoks ${CMAKE_SOURCE_DIR}/cli/debtoks debtoks)



set(SOURCE main.cpp debugger.cpp debtoks.cpp ${WORDFILELIST})

if(NOT NOLINEEDITING)
    set(SOURCE ${SOURCE} completer.cpp)
endif()

add_executable(angortcli ${SOURCE})

if(POSIXTHREADS)
    message("CLI POSIX threads enabled")
    target_compile_options(angortcli PUBLIC "-pthread")
endif(POSIXTHREADS)

if(EDITLINE_NOUNICODE)
    set_property(TARGET angortcli APPEND PROPERTY COMPILE_DEFINITIONS EDITLINE_NOUNICODE)
endif()

if(PROFILE)
    message("CLI profiling enabled")
    target_compile_options(angortcli PUBLIC "-pg")
    set_property(TARGET angortcli APPEND PROPERTY LINK_FLAGS -pg)
endif()


# we change the name of angortcli to angort, so it appears under that
# latter name in the build directory

set_target_properties(angortcli PROPERTIES
    OUTPUT_NAME angort)

set(LIBS angort ${PTHREAD})

if(NOT NOLINEEDITING)
    set(LIBS ${LIBS} ${LIBEDIT_LIBRARIES})
endif()
message("Libs: ${LIBS}")
target_link_libraries(angortcli ${LIBS})

install(TARGETS angortcli
    RUNTIME DESTINATION bin)
    
    
message("NLE: ${NOLINEEDITING}")
