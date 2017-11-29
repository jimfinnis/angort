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
    virtual void first()=0;
    // get the next item which matches, or NULL.
    virtual const char *next(const char *stringstart,int len)=0;
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
