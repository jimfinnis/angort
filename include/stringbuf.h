/**
 * @file string.h
 * @brief  Brief description of file.
 *
 */

#ifndef __STRINGBUF_H
#define __STRINGBUF_H

/// This wraps a string value converted from a Value. It may or
/// may not allocate a buffer, which will be freed when the value
/// deletes.

class StringBuffer {
private:
    bool allocated;
    const char *buf;
public:
    StringBuffer(const class Value *v);
    ~StringBuffer(){
        if(allocated){
            free((void *)buf);
            buf="FREED";
        }
    }
    
    const char *get() const{
        return buf;
    }
};


#endif /* __STRINGBUF_H */
