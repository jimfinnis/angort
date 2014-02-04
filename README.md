Angort is a (primarily) robot control language, inspired by Forth
but with many features designed to help the coder. These features
include:
* local variables and parameters in words
* garbage collected lists, ranges and iterators
* anonymous functions
* mutable copy closures

#Building 
Build with
    mkdir build
    cd build
    cmake ..
    make
    
Will build the executable in cli/angortcli





Define new words with
    :newword .... ;
as in Forth. If you want to recurse, forward-declare the word with "defer"


|param1,param2:local1,local2| to define params and locals


Conditions and loops:

if then else:
    (condition) if (true stuff) else (false stuff) then
infinite loop:
    { (loop stuff forever) }
leave a loop on some condition:
    { ... (condition) ifleave ... } 
    { ... leave ... }
Loop a given number of times:
    N times { ... }
    (on entry to each loop, stack top will be 0-(N-1). It must be restored to this
     by the corresponding loop end)
     


Stack manipulators:

dup, swap, drop, over

binary operators:
    + - * / = and or < >

Declare global called foo:
    global foo
Can now access variable using ?foo and !foo to get and put.

Alternatively, using a undefined variable whose name begins with
a capital letter immediately defines it as global.

Set constant called fish to 1.0:
    1.0 const fish


Fetch parameters, locals or globals with ?name
Store parameters, locals or globals with !name
so to increment a local called 'x':
    ?x 1+ !x

    
string literals : "foo"
float literals 123.0
int literals: 123

`word to stack a reference to a word (but not a native)

() to define anonymous words, and there is now full lexical closure if
required. @ to call the codeblock or closure on the stack (so "?foo@" will
call foo's value) There's also "call" which is the same as "@"

Note that closures are mutable copy closures:
> :mkcounter [:x] 0!x (?x 1+ . !x);
> mkcounter !R
> ?R@ 
1
> ?R@
2

works as expected, but
> :foo |:x| 4!x (?x 1+ !x)@ ?x .;
> foo
prints "4" and not "5", because the lambda modifies its closure's copy and not
the original


Iterators:

create a range iterator from 1 to 10 (inclusive), step 2:
    1 10 2 range
    
iterate over the range 1 to 10:
    1 10 1 range each { ... }

Print each item (i gets current item)
    1 10 1 range each { i . }

Nested:
    1 10 1 range each { 1 10 1 range each {j p " " p i .}}
(i gets innermost loop current, j the next loop out, k the third loop)

Ranges are objects in themselves, separate from their iterators,
so you can do

1 10 1 range !R
?R each { ?R each {j p " " p i.}}


Other words:

type        get the type of a value
reset       deletes everything
"foo" save  saves code and data to an image
"foo" load  loads code and data
list        lists all user words, constants and globals


gccount,abs,assert,disasm,debug,rawp..
