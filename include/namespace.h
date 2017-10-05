/**
 * @file namespace.h
 * @brief  Brief description of file.
 *
 */

#ifndef __ANGORTNAMESPACE_H
#define __ANGORTNAMESPACE_H

namespace angort {

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
        int idx = get(name);
        if(idx>=0){
            // throw Exception().set("name already exists in namespace: '%s'",name);
            return idx;
        }
        T *e = entries.append();
        idx = entries.getIndexOf(e);
        locations.set(name,idx);
        return idx;
    }
    
    T *getEnt(int idx){
        try{
            return entries.get(idx);
        }catch(Exception e){
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
    /// the entry will not be marked as imported in an import-all
    bool isPriv;
    /// the entry should be considered for matching when found
    /// in a scan of the imported namespaces.
    bool isImported;
    
    
    const char *spec; //!< specification value, may be NULL. Owned by this.
    
    NamespaceEnt(){
        isConst=false;
        isPriv=false;
        isImported=false;
        spec=NULL;
    }
    
    /// set a specification, which is copied in
    void setSpec(const char *s){
        spec = s?strdup(s):NULL;
    }
    
    void reset(){
        isConst=false;
        isPriv=false;
        if(spec){
            free((void *)spec);
            spec=NULL;
        }
        v.clr();
    }
};

/// this is a namespace - a set of Values indexed both by number
/// (inside opcodes) and name (when instructions are compiled).
/// A problem with this is that once an entry has been added, it cannot be removed.
/// This means (among other things) that syntax errors inside a word definition
/// will leave the word defined as None.


class Namespace : public NamespaceBase<NamespaceEnt> {
    friend class NamespaceManager;
    
    bool isImported; //!< the namespace is in the import list; add new public items to it
    
public:
    
    // each namespace has room for 32 names initially
    Namespace() : NamespaceBase<NamespaceEnt>(32) {
        isImported=false;
    }
    
    int addNonConst(const char *name,bool priv){
        int i = get(name);
        if(i<0)
            i = NamespaceBase<NamespaceEnt>::add(name);
        NamespaceEnt *e = getEnt(i);
        e->reset();
        e->isConst=false;
        e->isPriv=priv;
        e->isImported = (isImported && !priv);
        return i;
    }
    
    int addConst(const char *name,bool priv){
        int i = get(name);
        if(i<0)
            i = NamespaceBase<NamespaceEnt>::add(name);
        NamespaceEnt *e = getEnt(i);
        e->reset();
        e->isConst=true;
        e->isPriv=priv;
        e->isImported = (isImported && !priv);
        return i;
    }
    
    /// set the specification - a copy is made.
    void setSpec(int idx,const char *spec){
        getEnt(idx)->setSpec(spec);
    }
    
    Value *getVal(int i){
        return &(getEnt(i)->v);
    }
    
    /// mark all non-private items as imported - to really
    /// be imported, the namespace must be part of the imported
    /// namespaces list.
    
    void markAllImported(){
        for(int i=0;i<count();i++){
            NamespaceEnt *e = getEnt(i);
            if(!e->isPriv)
                e->isImported=true;
        }
    }
    
    
    
    /// wipe everything
    void clear(){
        for(int i=0;i<count();i++){
            NamespaceEnt *e = getEnt(i);
            e->reset();
        }
        NamespaceBase<NamespaceEnt>::clear();
    }
    
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
    
    /// return the full superindex for a name, or -1. Only
    /// returns public items if "allowPrivate" is false
    int getFromNamespace(Namespace *sp,const char *name,bool allowPrivate=false);
    
    /// if true, names are created private; otherwise names are
    /// created public.
    bool privNames;
    
    /// imported namespace list. Can't be a linked link because
    /// of the way namespaces move around, being in an arraylist.
    ArrayList<int> importedNamespaces;
    
public:
    NamespaceBase<Namespace> spaces; //< a namespace of namespaces!
    
    
    // the namespace system has room for 4 namespaces initially, but can grow.
    NamespaceManager() : importedNamespaces(4),spaces(4) {
        currentIdx=-1; // initially no namespace
        privNames=false;
        stack.setName("names");
    }
    
    //////////////////// manipulating namespaces /////////////////////
    
    /// create a new idx and return it
    int create(const char *name){
        int idx = spaces.add(name);
        // set the index in the ns we created
        spaces.getEnt(idx)->idx = idx;
        return idx;
    }
        
    /// push the current namespace onto the stack,
    /// and set a new current namespace.
    void push(int idx){
        stack.push(currentIdx);
        currentIdx = idx;
    }
    
    /// pop the namespace stack, returning the namespace
    /// we have just left - NOT the new current namespace.
    int pop(){
        int rv = currentIdx;
        currentIdx=stack.pop();
        return rv;
    }
    
    
    void clear(){
        for(int i=0;i<spaces.count();i++){
            spaces.getEnt(i)->clear();
        }
    }
    
    void list(){
        for(int i=0;i<spaces.count();i++){
            printf("Namespace %s %s\n",spaces.getName(i),
                   i==currentIdx ? "(current)":"");
            spaces.getEnt(i)->list();
        }
    }
    
    //////////////////// manipulating the current namespace ///////////
    
    int add(const char *name){
        Namespace *current = spaces.getEnt(currentIdx);
        return makeIndex(currentIdx,current->addNonConst(name,privNames));
    }
    
    int addConst(const char *name){
        Namespace *current = spaces.getEnt(currentIdx);
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
    
    void setSpec(int idx,const char *spec){
        int nsidx = getNamespaceIndex(idx);
        idx = getItemIndex(idx);
        
        spaces.getEnt(nsidx)->setSpec(idx,spec);
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
    
    /// given a value, try to find it in the namespaces! Slow.
    const char *getNameByValue(Value *v,char *out,int len);
        
    
    /// get a namespace
    Namespace *getSpaceByIdx(int i){
        return spaces.getEnt(i);
    }
    
    /// get a namespace, optionally create if not there
    Namespace *getSpaceByName(const char *s,bool createIfNotFound=false){
        int idx=spaces.get(s);
        if(idx<0){
            if(createIfNotFound)
                idx = create(s);
            else
                throw RUNT(EX_NOTFOUND,"cannot find namespace");
        }
        return spaces.getEnt(idx);
    }
    
    /// get a superindex by name - if there is a $, separate into
    /// namespace and name and resolve.
    
    // What needs to happen here:
    // check for explicit '$'. If so, search that namespace only. Otherwise search:
    // 1) current namespace
    // 2) imported namespaces (if scanImports is true)
    //
    
    int get(const char *name,bool scanImports=true);
    
    /// return true if a name is constant, false if it isn't or is
    /// not yet defined.
    bool isConst(const char *name,bool scanImports=true){
        int idx = get(name,scanImports);
        if(idx>=0)
            return getEnt(idx)->isConst;
        else
            return false;
    }
        
    
    
    /// import either all symbols or some symbols from a namespace.
    void import(int nsidx,ArrayList<Value> *lst);
};


}

#endif /* __NAMESPACE_H */
