/**
 * @file cliWords.cpp
 * @brief  Brief description of file.
 *
 */

#include "angort.h"



using namespace angort;

extern Value strippedArgVal;
Value promptCallback;

%name cli

%wordargs prompt C (function --) set prompt callback
Sets a function to be called to generate the prompt. This is a
function which has the form (gc stkcount promptchar -- string).
Note that any ANSI escape sequences should be wrapped with characters
001 and 002.
{
    promptCallback.copy(p0);
}

%word args (-- args) list of arguments, stripped of those handled by Angort
This returns the arguments passed to the program, except for those
which are parsed by Angort itself (e.g. "-d"). Any arguments after "--"
are not stripped and are not handled by Angort, and the "--" itself is
not added to this list.
{
    a->pushval()->copy(&strippedArgVal);
}
    
