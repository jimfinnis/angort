/**
 * @file value.cpp
 * @brief  Brief description of file.
 *
 */


#include "angort.h"
#include "hash.h"

using namespace angort;

void Value::dump(int depth){
    if(t == Types::tList){
        printf("[");
        Iterator<Value *> *iter = t->makeIterator(this);
        iter->first();
        for(;;){
            Value *v = iter->current();
            v->dump(depth+1);
            iter->next();
            if(!iter->isDone())printf(",");
            else break;
        }
        delete iter;
        printf("]");
    } else if(t == Types::tHash){
        printf("[%%");
        Hash *h = Types::tHash->get(this);
        Iterator<Value *> *iter = t->makeIterator(this);
        iter->first();
        for(;;){
            Value *v = iter->current();
            v->dump(depth+1);
            fputs(" ",stdout);
            if(h->find(v))
                h->getval()->dump(depth+1);
            else
                fputs("?ERROR?",stdout);
            iter->next();
            if(!iter->isDone())printf(",");
            else break;
        }
        delete iter;
        printf("]");
    } else if(t==Types::tString){
        putchar('"');
        fputs(toString().get(),stdout);
        putchar('"');
    } else if(t==Types::tSymbol){
        putchar('`');
        fputs(toString().get(),stdout);
    } else {
        fputs(toString().get(),stdout);
    }
    
    if(!depth)printf("\n");
}
