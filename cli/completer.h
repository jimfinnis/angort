/**
 * @file completer.h
 * @brief Autocompletion for editline/libedit
 *
 */

#ifndef __COMPLETER_H
#define __COMPLETER_H


class AutocompleteIterator {
public:
    virtual void first()=0;
    virtual const char *next()=0;
};


// call after EL_EDITOR set
void setupAutocomplete(EditLine *el,AutocompleteIterator *i);


#endif /* __COMPLETER_H */
