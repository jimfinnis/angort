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
protected:
    NamespaceBase(){}
    StringMap<int> locations;
    ArrayList<T> entries;
    
public:
    
    NamespaceBase(int initialSize) : entries(initialSize) {}
    
    void appendNamesToList(ArrayList<Value> *list){
        StringMapIterator<int> iter(&locations);
        for(iter.first();!iter.isDone();iter.next()){
            const char *name = iter.current()->key;
            Types::tString->set(list->append(),name);
        }
    }
    
    virtual int add(const char *name){
        if(get(name)>=0)
            throw Exception().set("name already exists in namespace: '%s'",name);
        T *e = entries.append();
        int idx = entries.getIndexOf(e);
        locations.set(name,idx);
        return idx;
    }
    
    T *getEnt(int idx){
        try{
            return entries.get(idx);
        }catch(ArrayListException e){
            return NULL;
        }
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
    bool isPriv;
    
    NamespaceEnt(){
        isConst=false;
        isPriv=false;
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
    }
    
    int addNonConst(const char *name,bool priv){
        int i = NamespaceBase<NamespaceEnt>::add(name);
        NamespaceEnt *e = getEnt(i);
        e->isConst=false;
        e->isPriv=priv;
        return i;
    }
    
    int addConst(const char *name,bool priv){
        int i = NamespaceBase<NamespaceEnt>::add(name);
        NamespaceEnt *e = getEnt(i);
        e->isConst=true;
        e->isPriv=priv;
        return i;
    }
    
    Value *getVal(int i){
        return &(getEnt(i)->v);
    }
    
    /// make a new entry in the namespace identical to
    /// the one passed in
    void copy(const char *name ,const NamespaceEnt *ent){
        int i = NamespaceBase<NamespaceEnt>::add(name);
        NamespaceEnt *e = getEnt(i);
        e->isConst = ent->isConst;
        e->isPriv = ent->isPriv;
        e->v.copy(&(ent->v));
    }
    
    /// wipe everything
    void clear(){
        for(int i=0;i<count();i++){
            NamespaceEnt *e = getEnt(i);
            e->v.clr();
            e->isConst=false;
            e->isPriv=false;
        }
        NamespaceBase<NamespaceEnt>::clear();
    }
    
    /// import all permitted entries into the given namespace
    void importAllTo(Namespace *dest);
    
    /// import a given entry into the given namespace (throw RUNT if not found)
    void importTo(Namespace *dest,const char *name);
    
    void list();
    
    int idx; //!< index within namespace manager's "spaces" namespace.
};

//#define MKIDX(i)
//#define GETNSIDX(i)
//#define GETITEMIDX(i)

/**
 * Handles all the namespaces, using a namespace of namespaces.
 * Deals in "superindices" - a superindex contains both the
 * index of the namespace, and the entry within that namespace.
 * There are methods for assembling and disassembling these.
 */

class NamespaceManager {
private:
    NamespaceBase<Namespace> spaces; //< a namespace of namespaces!
    
    int defaultIdx; //!< the index of the default namespace
    
    Namespace *current; //!< the namespace to which names are currently being added
    int currentIdx; //!< the index of the current namespace
    
    Stack<int,8> stack; //!< we maintain a stack of namespaces for when packages include others
    
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
    
    /// return the full superindex for a name, or -1
    int getFromNamespace(Namespace *sp,const char *name);
    
    /// if true, names are created private; otherwise names are
    /// created public.
    bool privNames;
    
public:
    
    // the namespace system has room for 4 namespaces initially, but can grow.
    NamespaceManager() : spaces(4) {
        privNames=false;
    }
    
    //////////////////// manipulating namespaces /////////////////////
    
    /// create a new idx and return it, optionally setting the initial,
    /// default namespace
    int create(const char *name,bool isdefault=false){
        int idx = spaces.add(name);
        // set the index in the ns we created
        spaces.getEnt(idx)->idx = idx;
        if(isdefault){
            defaultIdx = idx;
            currentIdx = idx;
            current = spaces.getEnt(defaultIdx);
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
        return makeIndex(currentIdx,current->addNonConst(name,privNames));
    }
    
    int addConst(const char *name){
        return makeIndex(currentIdx,current->addConst(name,privNames));
    }
    
    /// set whether created names are private - if so, they will not
    /// be imported
    void setPrivate(bool p){
        privNames = p;
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
        Namespace *sp = spaces.getEnt(nsidx);
        return sp->getEnt(idx);
    }
    
    const char *getName(int idx){
        int nsidx = getNamespaceIndex(idx);
        idx = getItemIndex(idx);
        
        return spaces.getEnt(nsidx)->getName(idx);
    }
    
    /// get a namespace
    Namespace *getSpaceByIdx(int i){
        return spaces.getEnt(i);
    }
    
    /// get a namespace
    Namespace *getSpaceByName(const char *s){
        int idx=spaces.get(s);
        if(idx<0)
            throw RUNT("cannot find namespace");
        return spaces.getEnt(idx);
    }
    
    /// get a superindex by name - if there is a $, separate into
    /// namespace and name and resolve.
    
    // What needs to happen here:
    // check for explicit '$'. If so, search that namespace only. Otherwise search:
    // 1) current namespace
    // 2) default namespace
    
    int get(const char *name);
    
    /// import either all symbols or some symbols from a namespace.
    void import(int nsidx,ArrayList<Value> *lst);
};




#endif /* __NAMESPACE_H */
