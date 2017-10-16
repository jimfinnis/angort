/**
 * @file nsid.h
 * @brief  Brief description of file.
 *
 */

#ifndef __NSID_H
#define __NSID_H


namespace angort {

/// Namespaces are identified by integers, but we don't want Angort
/// to be able to confuse NSIDs and integers. Therefore we have this
/// simple type.

class NSIDType : public Type {
public:
    NSIDType(){
        add("namespace","NSID");
    }
    /// get the value of v as an NSID
    int get(Value *v) const;
    /// set the value to the given NSID
    void set(Value *v,int f) const;
    
    /// get a hash key
    virtual uint32_t getHash(Value *v) const;
    
    /// are these two equal
    virtual bool equalForHashTable(Value *a,Value *b) const;
    
    virtual void toSelf(Value *out,const Value *v) const;
protected:
    virtual const char *toString(bool *allocated,const Value *v) const ;
};

}


#endif /* __NSID_H */
