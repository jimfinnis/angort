/**
 * @file string.cpp
 * @brief  Brief description of file.
 *
 */

#include "angort.h"


StringBuffer::StringBuffer(const Value *v){
    allocated=false;
    buf = v->t->toString(&allocated,v);
    wide = NULL;
}
