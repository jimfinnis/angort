Angort is a (primarily) robot control language, inspired by Forth
but with many features designed to help the coder. These features
include:
* local variables and parameters in words
* garbage collected lists, ranges and iterators
* anonymous functions
* mutable copy closures

##Building 
Build with

    mkdir build
    cd build
    cmake ..
    make
    
Will build the executable in cli/angortcli



##Basic use 

Start angort with 

    cli/angortcli

inside the build directory. You can now type words at the prompt. The two numbers in the prompt are the number of garbage-collectable objects in the system and the number of items on the stack.

Angort is a stack-based language: most words do things to the stack. For example,

    3

by itself will just put 3 on top of the stack.

    .
    
will print the value on top of the stack.

    3 4 + .

will print 7.

##Defining words

Define new words with

    :newword .... ;

as in Forth. If you want to recurse, forward-declare the word with "defer." Here are some example functions:

    :square dup * ;
    
This will duplicate the number on the stack and then multiply the top two numbers, leaving the result.

    4 square .
    
will print 16.

    defer factorial
    :factorial |x:|
        ?x 1 = if
            1
        else
            ?x ?x 1 - factorial *
        then
    ;
    
is the factorial function - there's a parameter "x" and a conditional there.

## Word parameters and local variables

Define these by putting a block of the form

    |param1,param2...:local1,local2...|
    
after the word name in the definition. Locals and parameters are exactly the same internally, but the values of parameters are popped off the stack when the word is called:

    :add |x,y:| ?x ?y + ;
    5 6 add .

will print 11.

We push the value of a local or parameter onto the stack with a question mark followed by the name. An exclamation work will pop the value from the stack and store it in the appropriate variable.

    :foo |x,y:z| ?x ?y + !z;
    
is a word which will add the two parameters, store them in the local z and then throw everything away.



##Conditions

If .. then .. else conditions look like

    <condition> if <true part> then
    
or

    <condition> if <true part> else <false part> then
    
##Loops
These are infinite unless we explicitly exit, and are delimited by {}:

    :loopsForever |:i| 0 !i { ?i dup . 1+ !i } ;
    loopsForever
    
will just print an incrementing count until you hit ^C. Take a few minutes to trace through it. To leave a loop, use the "leave" word. There's also the "ifleave" word, which leaves the loop if the value on top of the stack is true:

    :countTo |x:i| 0!i { ?i dup . ?x = ifleave ?i 1+ !i} ;
    10 countTo
    
will count from 0 to 10. A better way to define countTo is:

    :countTo |x:| 0 ?x 1 range each {i .}
    

Note that in "immediate mode", when you're not defining a word, a control construct like if..else..then or loops cannot span more than one line. Inside a word definition you can do what you like.
    
    
##Ranges and iterators
The "range" word pushes an integer range object onto the stack:

    <start> <end> <step> range
    
This can be stored in a variable, duplicated, printed (although you'll just get a range ID) and so on - it's just a value. "each {}" can then be used to create an "iterator loop" over the range:

    0 10 1 range each { i . }
    
The "i" word will put the current iterator's value on the stack. We can nest iterators:

    0 10 1 range each { "  " p i . 0 2 1 range each {i.}}
    
will show two nested loops. Note that 

    "  " p
    
That stacks a literal string (of two spaces), and then prints it without a newline.

Because each "each" has its own iterator, we can do the following:

    :foo |:r|
        0 10 1 range !r         # store a range
        ?r each {               # outer loop
            i .
            ?r each {           # inner loop
                i .
            }
        }
    ;
    
The range is used twice, with two separate iterators.

# To do
Everything below here is rough notes.
        
        
     


Stack manipulators:

    dup, swap, drop, over

binary operators:

    + - * / = and or < >

Declare global called foo:

    global foo

Can now access variable using ?foo and !foo to get and put.

Alternatively, using a undefined variable whose name begins with a capital letter immediately defines it as global.

Set constant called fish to 1.0:

    1.0 const fish



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
