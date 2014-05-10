/**
 * @file namespace.cpp
 * @brief  Brief description of file.
 *
 */
#include "angort.h"


void Namespace::list(){
    locations.listKeys();
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
        Namespace *sp = spaces.getEnt(spaceidx);
        return makeIndex(spaceidx,sp->get(dollar+1));
    }
    
    // default case - scan the namespace stack
    
    for(int i=0;i<stack.ct;i++){
        int nsidx = stack.peek(i);
        Namespace *sp = spaces.getEnt(nsidx);
        int iidx = sp->get(name);
        if(iidx>=0)
            return makeIndex(nsidx,iidx);
    }
    
    // if not there, check the imported spaces
    
    for(Namespace *sp = headImport;sp;sp=sp->nextImport){
        int iidx = sp->get(name);
        if(iidx>=0)
            return makeIndex(sp->idx,iidx);
    }
    
    // finally, check the default space
        
    return makeIndex(defaultIdx,defaultSpace->get(name));
}


void NamespaceManager::import(int nsidx,ArrayList<Value> *lst){
    // if there's a list, we copy the data into the default ns.
    // Otherwise, we add the namespace to the import list.
    
    Namespace *ns = spaces.getEnt(nsidx);
    printf("scanning ns %d\n",nsidx);
    if(lst){
        // go through the list
        ArrayListIterator<Value> iter(lst);
        for(iter.first();!iter.isDone();iter.next()){
            char buf[256];
            Value *v = iter.current();
            const char *s = v->toString(buf,256);
            // find the symbol in the namespace
            int idx = ns->get(s);
            if(idx<=0)
                throw RUNT("").set("cannot import '%s'",s);
            NamespaceEnt *ent = ns->getEnt(idx);
            defaultSpace->copy(s,ent);
        }
    }
}

