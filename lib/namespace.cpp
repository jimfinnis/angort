/**
 * @file namespace.cpp
 * @brief  Brief description of file.
 *
 */
#include "angort.h"
#include "file.h"
#include "ser.h"


void Namespace::list(){
    locations.listKeys();
}

void Namespace::visit(ValueVisitor *visitor){
    for(int i=0;i<entries.count();i++){
        getEnt(i)->v.receiveVisitor(visitor,getName(i));
    }
}

void Namespace::save(Serialiser *ser){
    File *f = ser->file;
    f->writeInt(entries.count());
    for(int i=0;i<entries.count();i++){
        NamespaceEnt *e = getEnt(i);
        f->writeString(getName(i));
        f->write16(e->isConst ? 1:0);
        e->v.save(ser);
    }
}

void Namespace::load(Serialiser *ser){
    char buf[1024];
          
    File *f = ser->file;
    int size = f->readInt();
    
    // we assume the set is clear; we might add hackage
    // later for when it isn't
    
    for(int i=0;i<size;i++){
        f->readString(buf,1024);
        int idx = add(buf);
        NamespaceEnt *e = getEnt(idx);
        e->isConst = (f->read16()!=0);
        e->v.load(ser);
    }
}


void NamespaceManager::save(Serialiser *ser){
    File *f = ser->file;
    f->write16(spaces.count());
    
    for(int i=0;i<spaces.count();i++){
        f->writeString(spaces.getName(i));
        spaces.getEnt(i)->save(ser);
    }
}

void NamespaceManager::load(Serialiser *ser){
    File *f = ser->file;
    int n = f->read16();
    
    for(int i=0;i<n;i++){
        char buf[256];
        f->readString(buf,256);
        
        // create namespace if it doesn't exist
        if(spaces.get(buf)<0)
            create(buf);
        
        // get it as current, and load it
        set(buf);
        Namespace *n = spaces.getEnt(spaces.get(buf));
        n->load(ser);
    }
}

