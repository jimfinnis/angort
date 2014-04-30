/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#ifndef __NONE_H
#define __NONE_H

/// The special 'none' type

class NoneType : public Type {
public:
    NoneType(){
        add("none","NONE");
    }
    
    virtual int toInt(const Value *v) const {
        return 0;
    }
    virtual const char *toString(char *outBuf,int len,const Value *v) const
    {
        strncpy(outBuf,"NONE",len);
        return outBuf;
    }
};



#endif /* __NONE_H */
