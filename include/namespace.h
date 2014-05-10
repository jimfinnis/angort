/**
 * @file namespace.h
 * @brief  Brief description of file.
 *
 */

#ifndef __NAMESPACE_H
#define __NAMESPACE_H

/// this is a core 'namespace', which is an integer-and-string keyed array of things.
/// You can't remove things from it, because existing integer keys would become invalid.

template <class T> class NamespaceBase {
private:
    NamespaceBase(){}
protected:
    StringMap<int> locations;
    ArrayList<T> entries;
    
public:
    NamespaceBase(int initialSize) : entries(initialSize) {}
    
    virtual int add(const char *name){
        if(get(name)>=0)
            throw Exception().set("name already exists in namespace: '%s'",name);
        T *e = entries.append();
        int idx = entries.getIndexOf(e);
        locations.set(name,idx);
        return idx;
    }
    
    T *getEnt(int idx){
        return entries.get(idx);
    }
    
    const char *getName(int i){
        return locations.getKey(i);
    }
    
    int get(const char *name){
        if(!locations.find(name))
            return -1;
        else
            return locations.found();
    }
    
    int count(){
        return entries.count();
    }
    
    /// wipe everything
    virtual void clear(){
        locations.clear();
        entries.clear();
    }
};
    

/// this is a namespace entry, containing the value itself
/// and whether it is a constant or not.
struct NamespaceEnt {
    Value v;
    bool isConst;
    
    NamespaceEnt(){
        isConst=false;
    }
};

/// this is a namespace - a set of Values indexed both by number
/// (inside opcodes) and name (when instructions are compiled).
/// A problem with this is that once an entry has been added, it cannot be removed.
/// This means (among other things) that syntax errors inside a word definition
/// will leave the word defined as None.


class Namespace : public NamespaceBase<NamespaceEnt> {
public:
    
    // each namespace has room for 32 names initially
    Namespace() : NamespaceBase<NamespaceEnt>(32) {
        nextImport=NULL;
    }
    
    virtual int add(const char *name){
        int idx = NamespaceBase<NamespaceEnt>::add(name);
        NamespaceEnt *e = getEnt(idx);
        e->isConst=false;
        return idx;
    }
    
    int addConst(const char *name){
        int idx = NamespaceBase<NamespaceEnt>::add(name);
        NamespaceEnt *e = getEnt(idx);
        e->isConst=true;
        return idx;
    }
    
    Value *getVal(int idx){
        return &(getEnt(idx)->v);
    }
    
    /// make a new entry in the namespace identical to
    /// the one passed in
    void copy(const char *name ,const NamespaceEnt *ent){
        int idx = NamespaceBase<NamespaceEnt>::add(name);
        NamespaceEnt *e = getEnt(idx);
        e->isConst = ent->isConst;
        e->v.copy(&(ent->v));
    }
    
    /// wipe everything
    void clear(){
        for(int i=0;i<count();i++){
            NamespaceEnt *e = getEnt(i);
            e->v.clr();
            e->isConst=false;
        }
        NamespaceBase<NamespaceEnt>::clear();
    }
    
    void list();
    /// used to link imported namespaces; you can't "unimport" a
    /// namespace.
    Namespace *nextImport;
    // sadly we need this for the import list, so we can make the superindex
    // for an item when we get
    int idx; 
};

#define MKIDX(i)
#define GETNSIDX(i)
#define GETITEMIDX(i)

class NamespaceManager {
private:
    NamespaceBase<Namespace> spaces; //< a namespace of namespaces!
    
    Namespace *defaultSpace;
    int defaultIdx;
    Namespace *current;
    int currentIdx;
    
    Stack<int,8> stack;
    
    Namespace *headImport; //!< head of imported namespaces list
    
    /// this will add a namespace to the imported chain;
    /// these are searched in order (most recent first) before
    /// the default space and after the stacked spaces.
    void import(Namespace *sp){
        sp->nextImport = headImport;
        headImport = sp;
    }
    
    
    /// make a superindex out of a namespace index and an item index
    inline int makeIndex(int nsi,int itemi){
        return itemi + (nsi<<14);
    }
    
    /// from a superindex, return the namespace index
    inline int getNamespaceIndex(int idx){
        return idx>>14;
        
    }
    
    /// from a superindex, return the item index
    inline int getItemIndex(int idx){
        return idx & ((1<<14)-1);
        
    }
public:
    
    // the namespace system has room for 4 namespaces initially, but can grow.
    NamespaceManager() : spaces(4) {}
    
    //////////////////// manipulating namespaces /////////////////////
    
    /// create a new idx and return it, optionally setting the initial,
    /// default namespace
    int create(const char *name,bool isdefault=false){
        int idx = spaces.add(name);
        // set the index in the ns we created
        spaces.getEnt(idx)->idx = idx;
        if(isdefault){
            defaultIdx = idx;
            defaultSpace = spaces.getEnt(idx);
            currentIdx = idx;
            current = defaultSpace;
        }
        return idx;
    }
    
    int getStackTop(){
        if(stack.ct>0)
            return stack.peek();
        else
            return -1;
    }
    
    void push(int idx){
        stack.push(idx);
        currentIdx = idx;
        current = spaces.getEnt(currentIdx);
    }
    
    int pop(){
        int idx=stack.pop();
        if(stack.ct>0)
            currentIdx=stack.peek();
        else
            currentIdx=defaultIdx;
        current = spaces.getEnt(currentIdx);
        return idx;
    }
    
    void clear(){
        for(int i=0;i<spaces.count();i++){
            spaces.getEnt(i)->clear();
        }
    }
    
    void list(){
        for(int i=0;i<spaces.count();i++){
            printf("Namespace %s\n",spaces.getName(i));
            spaces.getEnt(i)->list();
        }
    }
    
    //////////////////// manipulating the current namespace ///////////
    
    int add(const char *name){
        return makeIndex(currentIdx,current->add(name));
    }
    
    int addConst(const char *name){
        return makeIndex(currentIdx,current->addConst(name));
    }
    
    //////////////////// getting items across all namespaces //////////
    
    Value *getVal(int idx){
        int nsidx = getNamespaceIndex(idx);
        idx = getItemIndex(idx);
        
        return spaces.getEnt(nsidx)->getVal(idx);
    }
    
    NamespaceEnt *getEnt(int idx){
        int nsidx = getNamespaceIndex(idx);
        idx = getItemIndex(idx);
        
        return spaces.getEnt(nsidx)->getEnt(idx);
    }
    
    const char *getName(int idx){
        int nsidx = getNamespaceIndex(idx);
        idx = getItemIndex(idx);
        
        return spaces.getEnt(nsidx)->getName(idx);
    }
    
    /// get an index by name - if there is a $, separate into
    /// namespace and name and resolve.
    
    // What needs to happen here:
    // check for explicit '$'. If so, search that namespace only. Otherwise search:
    // 1) namespace stack
    // 2) imported namespaces
    // 3) default
    
    int get(const char *name);
    
    Value *getValFromNamespace(const char *space,int idx){
        int spaceidx = spaces.get(space);
        Namespace *sp = spaces.getEnt(spaceidx);
        return sp->getVal(idx);
    }
    
    
    /// import either all symbols (by adding to the list of imported
    /// namespaces) or some symbols (by adding those to the default
    /// namespace) from a namespace.
    void import(int nsidx,ArrayList<Value> *lst);
};




#endif /* __NAMESPACE_H */
