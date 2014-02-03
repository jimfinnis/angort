/**
 * \file
 * Brief description. Longer description.
 * 
 * \author $Author$
 * \date $Date$
 */


#ifndef __ASSERTIONS_H
#define __ASSERTIONS_H

class AssertionFailedException : public Exception {
public:
    AssertionFailedException(const char *s) : Exception(s){};
};


#endif /* __ASSERTIONS_H */
