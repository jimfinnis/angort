/**
 * @file completer.h
 * @brief Autocompletion for editline/libedit
 *
 */

#ifndef __COMPLETER_H
#define __COMPLETER_H


namespace completer {

// override this to create a class which iterates over possible
// strings, returning those which match the first "len" characters
// of "stringstart".

class Iterator {
public:
    // reset the iterator - next() will get the first item
    virtual void first(const char *stringstart,int len)=0;
    // get the next item which matches, or NULL.
    virtual const char *next()=0;
    
    // this is called when TAB is pressed, before anything else.
    // If it returns nonzero, the entire string is replaced by
    // the returned value and the returned buffer is deleted.
    // This is used to put the string into a canonical form
    // (~user files for example).
    // This must return an malloced string - it will be freed
    // by the caller.
    virtual const char *modString(const char *stringstart,int len){
        return NULL;
    }
    
    // by default, unambiguous completions have a space added.
    // This determines whether or not this should happen. Consider
    // filename completion - a completed file is fine, but a completed
    // unambiguous directory should pad with a space instead. Therefore
    // in this case we let the iterator do the padding itself.
    //
    virtual bool doSpacePadding(){return true;}
};

// call after EL_EDITOR has been set in your EditLine, to override
// the TAB key and connect everything up
void setup(EditLine *el,Iterator *defaultIter,
                       const char *wordBreakChars);

// call to delete the editor info and break the connection between
// the EditLine instance and the autocompleter
void shutdown(EditLine *el);

// set up an alternative completion iterator for a given argument
// number - if not called, the default passed into setup() will be
// used.
void setArgIterator(EditLine *el,
                    int arg,Iterator *i);

}

#endif /* __COMPLETER_H */
