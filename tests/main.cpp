/**
 * \file
 * This set of tests is used for things which can't be tested
 * by just firing Angort's interpreter at a file.
 * 
 * It could easily be empty.
 *
 * 
 * \author $Author$
 * \date $Date$
 */


#include "test.h"
#include <unistd.h>

Angort *newAngort(){
    Angort *a = new Angort();
}




TestSuite suite("tests");

int main(int argc,char *argv[]){
    if(argc<2)
        throw Exception("requires a working directory to be passed in, containing test files");
    
    if(chdir(argv[1]))
        throw Exception("cannot CD to test directory");
    
    if(suite.run() != suite.getCount())
        return 1;
    else
        return 0;
    
    
}
