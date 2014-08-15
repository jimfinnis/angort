/**
 * @file namespace.cpp
 * @brief  Brief description of file.
 *
 */
#include "angort.h"

namespace angort {

void Namespace::list(){
    locations.listKeys();
}

int NamespaceManager::getFromNamespace(Namespace *sp, const char *name,bool allowPrivate){
    int iidx = sp->get(name);
    if(iidx>=0){
        NamespaceEnt *ent = sp->getEnt(iidx);
        if(allowPrivate || ent->isImported)
            return makeIndex(sp->idx,iidx);
    }
    return -1;
}

int NamespaceManager::get(const char *name, bool scanImports){
    // does this contain a $?
    const char *dollar;
    if((dollar=strchr(name,'$'))){
        char buf[32];
        if(dollar-name > 32){
            throw RUNT("namespace name too long");
        }
        strncpy(buf,name,dollar-name);
        buf[dollar-name]=0;
        int spaceidx = spaces.get(buf);
        if(spaceidx<0)return -1; // no such namespace
        Namespace *sp = spaces.getEnt(spaceidx);
        int subidx = sp->get(dollar+1);
        if(subidx<0)return -1; // no such entry
        return makeIndex(spaceidx,sp->get(dollar+1));
    }
    
    // and this is for names in an unspecified space - 
    // first, scan the current list.
    
    int idx = getFromNamespace(spaces.getEnt(currentIdx),name,true);
    if(idx>=0)
        return idx;
    
    // now we scan the "imported namespaces" list
    if(scanImports){
        // we count in reverse, so we look at the most
        // recent namespaces first.
        for(int i=importedNamespaces.count()-1;i>=0;i--){
            int nsidx = *importedNamespaces.get(i);
            Namespace *ns = spaces.getEnt(nsidx);
            // again, only returns public items.
            int idx = getFromNamespace(ns,name);
            if(idx>=0)
                return idx;
        }
    }
    return -1; // not found
}


// this works by adding the namespace to the "imported namespaces"
// list. If there is no list, all public entries are marked as imported;
// if such a list exists, only those which are named are so marked.

void NamespaceManager::import(int nsidx,ArrayList<Value> *lst){
    
    Namespace *ns = spaces.getEnt(nsidx);
    *importedNamespaces.append() = nsidx;
    
    if(lst){
        // go through the list, marking those which are considered
        // imported.
        ArrayListIterator<Value> iter(lst);
        for(iter.first();!iter.isDone();iter.next()){
            Value *v = iter.current();
            int nidx = ns->get(v->toString().get());
            if(nidx>=0){
                NamespaceEnt *e = ns->getEnt(nidx);
                e->isImported=true;
            }
        }
    } else {
        // otherwise, mark all non-private entries as imported
        ns->markAllImported();
        // and also mark that subsequent public symbols should also
        // be imported
        ns->isImported=true;
    }
}

}
