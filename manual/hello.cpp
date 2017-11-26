#include <angort/angort.h>
#include <string>
#include <sstream>

// set the angort namespace.

using namespace angort;

// declare the name of the library and set a namespace for it -
// DO NOT use C++ namespace stuff after this.

%name hello

// say that we are making a shared library.

%shared

// "hello" takes a single argument, a string, which will automatically be
// converted into const char *p0. If the argument spec ("s") were longer, subsequent
// arguments would be converted into p1,p2... of the appropriate type.
// Type letters are 
// n - float
// d - double
// l - long int
// i - int
// c - codeblock/closure
// C - codeblock/closure/none
// v - any value
// y - string/symbol/none
// l - list
// h - hash
// A-Z - user-defined type (see example_complex/complex.cpp)

%wordargs hello s (name -- hello world string)
{
    std::stringstream s;
    s << "Hello " << p0 << ", how are you?";
    // "a" is the angort object, which is automatically passed in. The pushString()
    // method does exactly as its name implies, taking a const char *.
    a->pushString(s.str().c_str());
}

// initialisation function

%init
{
   fprintf(stderr,"Initialising HELLO plugin, %s %s\n",__DATE__,__TIME__);
}    


// optional shutdown function

%shutdown
{
   fprintf(stderr,"Closing HELLO plugin, %s %s\n",__DATE__,__TIME__);
}    
