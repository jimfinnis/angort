/**
 * @file string.h
 * @brief  Brief description of file.
 *
 */

#ifndef __ANGORTSTRINGBUF_H
#define __ANGORTSTRINGBUF_H

#include <wchar.h>
#include <stdlib.h>

namespace angort {

/// This wraps a string value converted from a Value. It may or
/// may not allocate a buffer, which will be freed when the value
/// deletes.

class StringBuffer {
private:
    bool allocated;
    const char *buf;
    wchar_t *wide;
    char tmp[33]; // twice the usual MB_LEN_MAX, +1.
public:
    StringBuffer(){
        buf=NULL;
    }
    
    void set(const class Value *v);
    
    StringBuffer(const class Value *v) {
        buf=NULL;
        set(v);
    }
    
    void clear(){
        if(buf){
            if(allocated){
                free((void *)buf);
            }
            if(wide)
                free((void *)wide);
            buf = NULL;
        }
    }
        
    ~StringBuffer(){
        clear();
    }
    
    const char *get() const{
        return buf;
    }
    
    /// allocate a wide buffer - use getWide() instead if you can
    /// to get a cached version. This is used by getWide() and for
    /// things like string slicing.
    wchar_t *getWideBuffer() const {
        mbstate_t state;
        memset(&state,0,sizeof(state));
        // get the size required
        const char *src = buf;
        int len = mbsrtowcs(NULL,&src,0,&state);
        // allocate
        wchar_t *s = (wchar_t *)malloc(sizeof(wchar_t)*(len+1));
        // and do the conversion
        src = buf;
        mbsrtowcs(s,&src,len+1,&state); // +1 for the null
        return s;
    }
    
    /// return a wide representation of a UTF-8 string,
    /// allocating a buffer (which is cached)
    
    const wchar_t *getWide(){
        if(!wide){
            wide = getWideBuffer();
        }
        return wide;
    }
};

}

#endif /* __STRINGBUF_H */
