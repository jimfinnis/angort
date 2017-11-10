/**
 * @file future.cpp
 * @brief  Brief description of file.
 *
 */

#include "angort.h"

using namespace angort;

namespace angort {
}


%name future

%wordargs slice vii (iterable start end -- iterable) produce a slice of a string or list
Return a slice of a string or list, returning another string or or list,
given the start and end positions. The end index is exclusive; that element
will not be included. Inappropriate types will throw ex$notcoll.
If length of the iterable is greater than the length required, then the
output will be truncated to the remainder of the iterable.
Negative indices count from just past the end, so -1 is the last
element. A zero end index thus means the end.
Empty results will be returned if the requested slice does not intersect
the iterable.
{
    Value iterable;
    iterable.copy(p0);
    int start = p1;
    int len = p2;
    
    Value *res = a->pushval();
    iterable.t->slice(res,&iterable,start,len);
}
