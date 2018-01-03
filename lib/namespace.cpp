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
            throw RUNT(EX_LONGNAME,"namespace name too long");
        }
        strncpy(buf,name,dollar-name);
        buf[dollar-name]=0;
        int spaceidx = spaces.get(buf);
        if(spaceidx<0)return -1; // no such namespace
        Namespace *sp = spaces.getEnt(spaceidx);
        int subidx = sp->get(dollar+1);
        if(subidx<0)return -1; // no such entry
        if(!sp->getEnt(subidx)->isPriv)
            return makeIndex(spaceidx,sp->get(dollar+1));
        else
            return -1;
        
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

const char *NamespaceManager::getFQN(int idx,char *buf,int len){
    int nsidx = getNamespaceIndex(idx);
    int vidx = getItemIndex(idx);
    
    Namespace *ns = spaces.getEnt(nsidx);
    const char *nsname = spaces.getName(nsidx);
    const char *vname = ns->getName(vidx);
    
    snprintf(buf,len,"%s$%s",nsname,vname);
    buf[len-1]=0; // make sure terminated
    return buf;
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
            } else {
                throw RUNT(EX_NOTFOUND,"").set("cannot find '%s' in namespace",
                                            v->toString().get());
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


const char *NamespaceManager::getNameByValue(Value *v,char *out,int len){
    // first we need to iterate the namespaces
    
    for(int nsidx=0;nsidx<spaces.count();nsidx++){
        const char *nsName = spaces.getName(nsidx);
        Namespace *ns = spaces.getEnt(nsidx);
        
        // now we need to do a "find by value" in each space
        for(int i=0;i<ns->count();i++){
            const char *name = ns->getName(i);
            NamespaceEnt *ent = ns->getEnt(i);
            
            // we're going to use some special cases here.
            Value *c = &ent->v;
            bool cmp=false;
            if(v->t == c->t){
                if(v->t == Types::tNative)
                    cmp = v->v.native == c->v.native;
                else if(v->t == Types::tProp)
                    cmp = v->v.property == c->v.property;
                else if(v->t == Types::tList)
                    cmp = v->v.list == c->v.list;
                else if(v->t == Types::tHash)
                    cmp = v->v.hash == c->v.hash;
                else cmp = v->equalForHashTable(c);
            }
            if(cmp){
                snprintf(out,len,"%s$%s",nsName,name);
                return out;
            }
        }
    }
    strcpy(out,"??");
    return out;
    
}


}
