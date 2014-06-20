/**
 * @file string.h
 * @brief  Brief description of file.
 *
 */

#ifndef __STRINGBUF_H
#define __STRINGBUF_H

#include <wchar.h>

/// This wraps a string value converted from a Value. It may or
/// may not allocate a buffer, which will be freed when the value
/// deletes.

class StringBuffer {
private:
    bool allocated;
    const char *buf;
    wchar_t *wide;
public:
    StringBuffer(const class Value *v);
    ~StringBuffer(){
        if(allocated){
            free((void *)buf);
        }
        if(wide)
            free((void *)wide);
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


#endif /* __STRINGBUF_H */
