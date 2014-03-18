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
};



#endif /* __NONE_H */
