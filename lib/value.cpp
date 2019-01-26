/**
 * @file value.cpp
 * @brief  Brief description of file.
 *
 */


#include "angort.h"
#include "hash.h"

using namespace angort;

inline void strappend(char **stream,const char *s){
    int olen = *stream ? strlen(*stream) : 0;
    int slen = strlen(s);
    char *newstr = (char *)malloc(olen+slen+1);
    if(*stream)memcpy(newstr,*stream,olen);
    memcpy(newstr+olen,s,slen);
    newstr[slen+olen]=0;
    if(*stream)free(*stream);
    *stream=newstr;
}

void Value::dump(char **str,int depth){
    if(!depth)*str=NULL;
    if(t == Types::tList){
        strappend(str,"[");
        Iterator<Value *> *iter = t->makeIterator(this);
        iter->first();
        for(;;){
            Value *v = iter->current();
            v->dump(str,depth+1);
            iter->next();
            if(!iter->isDone())strappend(str,",");
            else break;
        }
        delete iter;
        strappend(str,"]");
    } else if(t == Types::tHash){
        strappend(str,"[%");
        Hash *h = Types::tHash->get(this);
        Iterator<Value *> *iter = t->makeIterator(this);
        iter->first();
        for(;;){
            if(iter->isDone())break; // ugly loop here, two breaks
            
            Value *v = iter->current();
            v->dump(str,depth+1);
            strappend(str," ");
            if(h->find(v))
                h->getval()->dump(str,depth+1);
            else
                fputs("?ERROR?",stdout);
            iter->next();
            if(!iter->isDone())strappend(str,",");
            else break;
        }
        delete iter;
        strappend(str,"]");
    } else if(t==Types::tString){
        strappend(str,"\"");
        strappend(str,toString().get());
        strappend(str,"\"");
    } else if(t==Types::tSymbol){
        strappend(str,"`");
        strappend(str,toString().get());
    } else {
        strappend(str,toString().get());
    }
    
//    if(!depth)strappend(str,"\n");
}


