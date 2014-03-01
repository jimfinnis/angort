/**
 * @file namespace.h
 * @brief  Brief description of file.
 *
 */

#ifndef __NAMESPACE_H
#define __NAMESPACE_H

/// this is a namespace - a set of values indexed both by number
/// (inside opcodes) and name (when instructions are compiled).


class Namespace {
    StringMap<int> locations;
    ArrayList<Value> values;
    
public:
    Namespace() : values(32) {
    }
    
    int add(const char *name){
        Value *v = values.append();
        int idx = values.getIndexOf(v);
        locations.set(name,idx);
        return idx;
    }
    
    Value *get(int idx){
        return values.get(idx);
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
        return values.count();
    }
    
    /// wipe everything
    void clear(){
        for(int i=0;i<values.count();i++){
            values.get(i)->clr();
        }
        locations.clear();
        values.clear();
    }
    
    /// visit all the elements
    void visit(ValueVisitor *visitor);
    void list();
    void load(Serialiser *ser);
    void save(Serialiser *ser);
};


#endif /* __NAMESPACE_H */
