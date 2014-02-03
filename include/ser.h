#ifndef __SER_H
#define __SER_H

#include <stdio.h>

struct FixupEnt {
    Type *t;
    const void *v;
};


/// class which handles controlling the serialisation process.
/// Angort creates one of these, and serialises to a file through it.
/// Deserialisation is also managed here.

class Serialiser {
    friend class BuildFixupTableVisitor;
    friend class ResolveFixupTableVisitor;
private:
    
    /// add an entry to the fixup table - this maps a new integer
    /// onto a void pointer, which is to the value of a reference
    /// type (such as a string or object). It's the v field of the
    /// value. Will NOT do anything if the fixup already exists,
    /// and will return false.
    bool createFixup(Type *t,const void *v);
    
    /// the fixup table, mapping integers onto
    /// the contents of objects
    ArrayList<FixupEnt> *fixups;
    
    bool isInFixups(const void *v){
        for(int i=0;i<fixups->count();i++){
            if(fixups->get(i)->v == v)
                return true;
        }
        return false;
    }
    
    
    void saveFixups();
    void loadFixups();
    
public:
    
    /// pointer to main angort object itself
    class Angort *angort;
    /// pointer to file object
    class File *file;
    
    /// get the fixup ID for some data
    uint32_t getFixupByData(const void *v);
    /// get the fixup by ID
    FixupEnt *getFixupByID(uint32_t id){
        return fixups->get(id);
    }
    
    /// get the data for some fixup ID and check the type
    const char *resolveFixup(Type *t, uint32_t id){
        FixupEnt *e=fixups->get(id);
        if(e->t != t)throw WTF;
        return (const char*)e->v;
    }
    
    
    /// walk some code, adding fixups for opcodes which have data.
    /// Recurses if there are anonymous code blocks.
    void createCodeFixups(const class CodeBlock *c);
    
    /// walk some code, resolving fixups
    /// Recurses if there are anonymous code blocks.
    void resolveCodeFixups(class CodeBlock *c);
    
    void load(class Angort *a,const char *name);
    void save(class Angort *a,const char *name);
};


#endif /* __SER_H */
