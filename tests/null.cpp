/**
 * \file
 * An empty, dummy test.
 *
 * 
 * \author $Author$
 * \date $Date$
 */

#include "test.h"

class NullTest : public Test {

public:    
    NullTest() : Test("Null") {
        suite.add(this);
    }
    
    virtual void run(Angort *a){
        Value v,w;
        Types::tInteger->set(&w,20);
        v.copy(&w);
    }
};

NullTest Null;
