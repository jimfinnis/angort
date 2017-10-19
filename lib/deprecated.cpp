/**
 * @file deprecated.cpp
 * @brief  Brief description of file.
 *
 */

#include "angort.h"

using namespace angort;

namespace angort {
}


%name deprecated

%wordargs slice vii (iterable start len -- iterable) produce a slice of a string or list
Return a slice of a string or list, returning another string or or list,
given the start and length. Inappropriate types will throw ex$notcoll.
If length of the iterable  is greater than the length requested, then the
slice will go to the end. If the start<0 it is counted from the end.
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
