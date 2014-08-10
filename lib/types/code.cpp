/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */
#include "angort.h"

#include "opcodes.h"

namespace angort {

void CodeType::set(Value *v,const struct CodeBlock *cb){
    v->clr();
    v->v.cb=cb;
    v->t = Types::tCode;
}


}
