/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#ifndef __ANGORTSYMBOL_H
#define __ANGORTSYMBOL_H

#include "../lock.h"

namespace angort {

/// symbol (i.e. string identified by ID) type

class SymbolType : public Type, public Lockable {
public:
    SymbolType(){
        add("symbol","SYMB");
        makeStringable();
    }
    
    /// delete all symbols
    static void deleteAll();
    
    /// get or create a new symbol
    static int getSymbol(const char *s);
    
    /// return false if the symbol does not exist
    static bool exists(const char *s);
    
    static const char *getString(int id);
    
    /// get the string value
    const char *get(const Value *v) const;
    
    /// set the symbol from its integer value
    void set(Value *v,int i);
    
    /// get a hash key
    virtual uint32_t getHash(Value *v)const;
    
    /// are these two equal
    virtual bool equalForHashTable(Value *a,Value *b)const;
    virtual int toInt(const Value *v) const;
protected:    
    virtual const char *toString(bool *allocated,const Value *v) const;
};

}
#endif /* __SYMBOL_H */
