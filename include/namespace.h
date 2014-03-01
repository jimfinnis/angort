/**
 * @file namespace.h
 * @brief  Brief description of file.
 *
 */

#ifndef __NAMESPACE_H
#define __NAMESPACE_H

struct NamespaceEnt {
    Value v;
    bool isConst;
    
    NamespaceEnt(){
        isConst=false;
    }
};

/// this is a namespace - a set of values indexed both by number
/// (inside opcodes) and name (when instructions are compiled).
/// A problem with this is that once an entry has been added, it cannot be removed.
/// This means (among other things) that syntax errors inside a word definition
/// will leave the word defined as None.


class Namespace {
    StringMap<int> locations;
    ArrayList<NamespaceEnt> entries;
    
public:
    Namespace() : entries(32) {
    }
    
    int add(const char *name){
        NamespaceEnt *e = entries.append();
        e->isConst=false;
        int idx = entries.getIndexOf(e);
        locations.set(name,idx);
        return idx;
    }
    
    int addConst(const char *name){
        NamespaceEnt *e = entries.append();
        e->isConst=true;
        int idx = entries.getIndexOf(e);
        locations.set(name,idx);
        return idx;
    }
    
    Value *get(int idx){
        return &(entries.get(idx)->v);
    }
    
    NamespaceEnt *getEnt(int idx){
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
    void clear(){
        for(int i=0;i<entries.count();i++){
            NamespaceEnt *e = entries.get(i);
            e->v.clr();
            e->isConst=false;
        }
        locations.clear();
        entries.clear();
    }
    
    /// visit all the elements
    void visit(ValueVisitor *visitor);
    void list();
    void load(Serialiser *ser);
    void save(Serialiser *ser);
};


#endif /* __NAMESPACE_H */
