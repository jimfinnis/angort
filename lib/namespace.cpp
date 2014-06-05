/**
 * @file namespace.cpp
 * @brief  Brief description of file.
 *
 */
#include "angort.h"


void Namespace::list(){
    locations.listKeys();
}

int NamespaceManager::getFromNamespace(Namespace *sp, const char *name){
    int iidx = sp->get(name);
    if(iidx>=0)
        return makeIndex(sp->idx,iidx);
    else
        return -1;
}

int NamespaceManager::get(const char *name){
    // does this contain a $?
    const char *dollar;
    if(dollar=strchr(name,'$')){
        char buf[32];
        if(dollar-name > 32){
            throw RUNT("namespace name too long");
        }
        strncpy(buf,name,dollar-name);
        buf[dollar-name]=0;
        int spaceidx = spaces.get(buf);
        if(spaceidx<0)return -1; // no such namespace
        Namespace *sp = spaces.getEnt(spaceidx);
        return makeIndex(spaceidx,sp->get(dollar+1));
    }
    
    // and this is for names in an unspecified space
    
    // first, scan the current list
    
    int idx = getFromNamespace(current,name);
    if(idx>=0)
        return idx;
    
    // finally, check the default space and error 
        
    return getFromNamespace(defaultSpace,name);
}


void NamespaceManager::import(int nsidx,ArrayList<Value> *lst){
    // if there's a list, we copy the data into the default ns.
    // Otherwise, we add the namespace to the import list.
    
    Namespace *ns = spaces.getEnt(nsidx);
    if(lst){
        // go through the list
        ArrayListIterator<Value> iter(lst);
        for(iter.first();!iter.isDone();iter.next()){
            char buf[256];
            Value *v = iter.current();
            const char *s = v->toString(buf,256);
            ns->importTo(defaultSpace,s);
        }
    } else {
        // import everything which is permitted
        ns->importAllTo(defaultSpace);
    }
}

void Namespace::importAllTo(Namespace *dest){
    StringMapIterator<int> iter(&locations);
    for(iter.first();!iter.isDone();iter.next()){
        const char *name = iter.current()->key;
        int loc = iter.current()->value;
        NamespaceEnt *ent = entries.get(loc);
        if(!ent->isPriv) // don't import privates
            dest->copy(name,ent);
    }
}


void Namespace::importTo(Namespace *dest,const char *name){
    int idx = get(name);
    if(idx<0)
        throw RUNT("").set("cannot import '%s'",name);
    NamespaceEnt *ent = getEnt(idx);
    if(!ent->isPriv)
        dest->copy(name,ent); // we do not import privates
}
