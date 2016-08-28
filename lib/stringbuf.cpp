/**
 * @file string.cpp
 * @brief  Brief description of file.
 *
 */

#include "angort.h"

namespace angort {


void StringBuffer::set(const Value *v){
    if(buf)clear();
    allocated=false;
    buf = v->t->toString(&allocated,v);
    wide = NULL;
}


}
