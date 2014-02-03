/**
 * @file cvset.cpp
 * @brief  Brief description of file.
 *
 */
#include "angort.h"
#include "file.h"
#include "ser.h"


void ContiguousValueSet::list(){
    StringMapIterator<int>iter(&locations);
    int lengthSoFar=0;
    for(iter.first();!iter.isDone();iter.next()){
        const char *s = iter.current()->key;
        lengthSoFar+=strlen(s);
        if(lengthSoFar>60){
            puts("\n");
            lengthSoFar=0;
        }
        printf("%s ",s);
    }
    if(lengthSoFar)puts("");
}

void ContiguousValueSet::visit(ValueVisitor *visitor){
    for(int i=0;i<values.count();i++){
        Value *v = values.get(i);
        v->receiveVisitor(visitor,getName(i));
    }
}

void ContiguousValueSet::save(Serialiser *ser){
    File *f = ser->file;
    f->writeInt(values.count());
    for(int i=0;i<values.count();i++){
        f->writeString(getName(i));
        values.get(i)->save(ser);
    }
}

void ContiguousValueSet::load(Serialiser *ser){
    char buf[1024];
          
    File *f = ser->file;
    int size = f->readInt();
    
    // we assume the set is clear; we might add hackage
    // later for when it isn't
    
    for(int i=0;i<size;i++){
        f->readString(buf,1024);
        int idx = add(buf);
        Value *v = get(idx);
        v->load(ser);
    }
}


