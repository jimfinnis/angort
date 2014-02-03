/**
 * \file
 * Brief description. Longer description.
 *
 * 
 * \author $Author$
 * \date $Date$
 */

#include "angort.h"
#include "assertions.h"

%name test

%word assert   (bool --)
{
    if(!a->popInt())
        throw(AssertionFailedException("assert"));
}

