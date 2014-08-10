/**
 * @file string.cpp
 * @brief  Brief description of file.
 *
 */

#include "angort.h"

namespace angort {


StringBuffer::StringBuffer(const Value *v){
    allocated=false;
    buf = v->t->toString(&allocated,v);
    wide = NULL;
}


}
