Angort is a (primarily) robot control language, inspired by Forth
but with many features designed to help the coder. These features
include:
* local variables and parameters in words
* garbage collected lists, ranges and iterators
* anonymous functions
* mutable copy closures

##Some examples
Functions in Angort are called "words", borrowing the Forth terminology. Here are
some of the word definitions I use to control our ExoMars rover prototype:

    # set a constant range for the wheel numbers
    1 6 range const wheels

    # a word to set the drive speed on all wheels
    :setdriveall |speed:| wheels each { ?speed i!drive };

    # forwards and backwards control, and stop
    :f 1500 setdriveall;
    :b -1500 setdriveall;
    :s 0 setdriveall;

    # set the back wheels to turn one way and the front wheels to turn
    # the opposite way
    :turn |angle:start| 
        ?angle dup 1!steer 2!steer
        ?angle neg dup 5!steer 6!steer
        0 dup 3!steer 4!steer;
        
        
With these words, we can control the rover:

    f 10 delay s        # drive forwards for 10 seconds and then stop
    20 t 4 delay        # turn the wheels and wait for completion
    f 10 delay s        # and another 10 seconds drive
    
The actual rover control words are more complex, doing things like waiting
until the rover steering positions actually match the requested angles
before proceeding.

We can also do complex things with lists and anonymous functions. Here is
a word for summing a iterable value (such as a list or a range):

    :sum |list:| 0 ?list (+) inject;
    
so we can do

    [1,2,3,4,5,6] sum .
    
or

    0 1000 range sum .

or even

    1 1000 range (dup *) map sum .
    
to print the sum of the squares of the first 1000 integers.


##Building 
Build with

    mkdir build
    cd build
    cmake ..
    make
    
Will build the executable in cli/angortcli



##Immediate mode

Start angort with 

    cli/angortcli

inside the build directory. You are now in "immediate mode" and can type words at the prompt. The two numbers in the prompt are the number of garbage-collectable objects in the system and the number of items on the stack.

Note that in "immediate mode", when you're not defining a new word (i.e. a function), control constructs like if..else..then or loops cannot span more than one line. Inside a word definition you can do what you like.

##The stack

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
    
will print 16. As mentioned above, you can use multiple lines for a word definition:

    defer factorial
    :factorial |x:|
        ?x 1 = if
            1
        else
            ?x ?x 1 - factorial *
        then
    ;
    
is the factorial function - there's a parameter "x" and a conditional there, and I'll cover those in a moment.

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
    
will count from 0 to 10. As an aside, a better way to define countTo is to use a range (see below):

    :countTo |x:| 0 ?x range each {i .}
        
    
##Ranges and iterators
The "range" and "srange" words push integer range objects onto the stack:

    <start> <end> range
    <start> <end> <step> srange
    
The only difference is that srange takes a step value. The step can be positive or negative. In
the normal range word, the step is 1 or -1 depending on whether the start is less than or greater
than the end value.

A range can be stored in a variable, duplicated, printed (although you'll just get a range ID) and so on - it's just a value. "each {}" can then be used to create an "iterator loop" over the range:

    0 10 range each { i . }
    
The "i" word will put the current iterator's value on the stack. We can nest iterators:

    0 10 range each { "  " p i . 0 2 range each {i.}}
    
will show two nested loops. Note that 

    "  " p
    
That stacks a literal string (of two spaces), and then prints it without a newline.

Because each "each" has its own iterator, we can do the following:

    :foo |:r|
        0 10 range !r         # store a range
        ?r each {               # outer loop
            i .
            ?r each {           # inner loop
                i .
            }
        }
    ;
    
The range is used twice, with two separate iterators.

We also have floating point ranges, using the frange word:
   
    0 1 0.1 frange each {i.}
    



### Nested loops
In a nested loop, it's possible to access the current variables of the outer loops by using "j" and "k":

    :foo |:r|
        0 10 range !r         # store a range
        ?r each {               # outer loop
            ?r each {           # inner loop
                i j + .         # add inner and outer iterators
            }
        }
    ;



## Globals and constants
Global variables are defined in two ways. The "polite" way is to use the global keyword, which creates a new global with that name. Once defined, you use globals the same way as local variables:

    global foo
    5 !foo
    ?foo .

will print 5. The other way to define globals is simply to access a variable whose name starts with a capital letter. If no global or local exists with that name, a new global is defined (initially with the special 'none' value):

     5 !Foo
     ?Foo .

Constants are similar to globals, but with three differences:
* they are defined and set using the const keyword - this will pop a value off the stack and set the new constant to that value;
* they can only be read, and cannot be redefined;
* they do not require the ? sigil to access them.

Here's an example:

     3.1415927 const pi
     :degs2rads pi 180.0 / * ;

## Stack manipulators
There are a number of words whose sole purpose is to manipulate the stack. To explain these, I'll use the bracket notation from Forth:

###Bracket notation
We describe the action of a word in terms of two states: the state before the word, and the state afterwards. These are written in a pair of brackets, separated by --, with the top of the stack to the right. For example, the "dup" word duplicates the value on top of the stack. We write this as

    (a -- a a)

The "." word pops an item off the stack, so its action as far as the stack is concerned is

    (a --)

(the bracket notation doesn't describe what a word does by way of side effects.) The "range" word takes three values and turns them into a range:

    (start end  -- range)

Our stack manipulators are:

word | action
--------|----------
dup | (a -- a a)
drop | (a --)
swap | (a b -- b a)
over | (a b -- a b a)

I've not implemented words like roll, nip, and tuck used in standard Forths because the local variable system means you really shouldn't need them. I wasn't sure about "over" to be honest.
## Types and literals
* Primitive types are 32 bit signed integers, 32 bit floats and strings.
* Literal integers are numbers without a decimal point - they may also be
suffixed with a base character, one of "dxhbo": "16x" or "16h" will be interpreted
as "16 in hexadecimal".
* Literal floats have decimal points
* Strings are a reference counted and immutable (copy on write)
* Other types include lists, functions (and closures), ranges and (internally) iterators.

## Binary operators
In the following operations, these conversions take place:
* if one of the operands is a string, the other will be converted to a string. Only "+" and comparison operators will be valid.
* If one of the operands is a float and the other is an integer, the integer will be converted to a float and the result will be a float.
* The comparison operators will do identity checks on objects (lists, ranges etc.), not deep comparisons.
* The comparison functions actually return integers, with non-zero indicating falsehood.

word | action|notes
-----|----|-----
+    | (a b -- a+b)|
-    | (a b -- a-b)|
*    | (a b -- a-b)|
/    | (a b -- a/b)|
%    | (a b -- a%b) | remainder ("mod") operator
>    | (a b -- a>b)| string comparison works as expected
<    | (a b -- a<b)| string comparison works as expected
=    | (a b -- a=b)| string comparison works as expected
!=   | (a b -- a!=b)| string comparison works as expected

## Functional stuff

Anonymous functions are defined with brackets, which will push an object representing that function (and any closure created) onto the stack. This can then be called with "call" or "@" for short. Such functions may have parameters and local variables.

For example, here's a function to run a function over a range of numbers, printing the result:

    :over1to10 |func:|
        1 10 range each { i ?func@ . } ;
        
and here's how it could be used to show the squares of the numbers:

    (|x:| ?x ?x *) over1to10
            
If we want to stack a reference to a word instead of running it, we can precede the word with a backtick:

    :square dup * ;    # more efficient than the above anonymous!
    `square over1to10

This will NOT WORK with builtin functions, however.

### Closures

Anonymous functions can refer to variables in their enclosing function, in which case a closure is created to store the value when the enclosing function exits. This closure is mutable - its value can be changed by the anonymous function. For example:

    :mkcounter |:x|     # declare a local variable x
        0!x             # set it to zero
        (               # create a function
            ?x dup .    # which prints the local
            1+ !x       # and increments it
        );
    mkcounter !F        # run it and store the returned function+closure

Now, if we run

    ?F @
    
a few times, we'll get an incrementing count - the value in the closure persists and is being incremented. We can call mkcounter several times and each time we'll get a new closure.

It's important to note that closures are copy closures - the anonymous function makes a copy of the local, and all changes inside the anonymous function happen to that copy, not the local in the parent:

    :foo |:x| 4!x       # store 4 in the local x
        (10 !x)         # store 10 in the closure's version of x
        @               # run the anonymous function
        ?x .            # this will print 4, because the local hasn't changed.
        ;
        
This is slightly annoying behaviour, but rather difficult to change given Angort's simple syntax.

##Lists
A list is defined by enclosing Angort expressions in square brackets separated by commas. The result is a list on the stack. Lists can be iterated:

    []                      # creates an empty list
    [1,2,3]                 # creates a list of three integers
    ["foo",bar"] each {i.}  # iterates over a list
    
Lists can contain lists, and can of course be stored in variables. Lists are mutable, and members can be set and retrieved using the "put" and "get" words. The words for lists are:

name | stack action | side-effects and notes
-----|--------------|----------
[    | (-- list)    | creates a new list
,    | (list item -- list) | appends an item to the list
]    | (list item -- list) | appends an item to the list
get | (n list -- item) | get the nth item from the list
put | (item n list --) | set the nth item in the list
remove | (n list -- item) | remove and return the nth item
shift | (list -- item) | remove and return the first item
unshift | (item list --) | prepend an item
pop | (list -- item) | remove and return the last item
push | (item list --) | append an item
map | (iter func -- list) | apply a function to an iterable, giving a list
inject | (start iter func -- result) | set an internal value (the accumulator) to "start", then iterate, applying the function (which must take two arguments) to the accumulator and the iterator's value, setting the accumulator to this new value before moving on.

As an example, here's an Angort implementation of the map function (which is actually
defined in as a native word):

    :amap |list,func:| [] ?list each { i ?func@ ,} ;
    
With this, we can map over any iterable to produce a list. Here's how you might use the map word
to multiply all the numbers between 0 and 10 by 100:

    0 10 range (100*) map each {i.}
        
##Some other builtin words

name | stack action | side-effects and notes
-----|--------------|----------
type | (x -- type) | get a string giving the type of a value
isnone | (v -- bool) | return nonzero if value is None
reset | (--)    | delete everything
save | (filename --) | save the running image (words, globals etc.) to a file
load | (filename --) | reset and load an image (words, globals etc.) from a file
list | (--) | list all defined words (including builtins), locals and constants
clear | (--) | clear the stack entirely
debug | (bool --) | turn debugging on or off
neg | (n -- -n) | negate an integer or float
abs | (n -- abs(n)) | absolute value
p | (x --) | print without newline
nl | (--) | print just a newline


There are quite a few more. To get a list of all the builtin words, use "list";
and to get help on an individual word, use 

    "word" help

If you're interested in extending Angort, look in the std.*.cpp files in angort/lib and angort/cli for
word definitions, and note how they're defined in a special variant of C++.
