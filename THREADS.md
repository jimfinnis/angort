# Threads.

## Threads, GC and global access

Because threads can theoretically modify any object, to make it work
with GC I've had to slap global locks all around the GC system.
And since every time a GC object is accessed it uses the GC system,
these locks run an awful lot. This makes Angort a bit on the inefficient
side when threading. If you run a pure Angort app with lots of
threads, you might notice that the CPU utilisation isn't what it might be.

It's a bit better than it once was because I've removed the lock
around the `clr()` method of `Value`. This isn't ideal, but this method
is called a huge amount - whenever a value overwrites another value.
In consequence, you MUST NOT access a global in a thread without
using a thread library mutex.

## Thread library

The thread library is not imported by default, but is now built into
Angort. Create
a thread using `thread$create`, which takes an argument and a function
(*not* a closure - that would be very hard indeed). The argument is
copied onto the thread's stack - all threads get a separate runtime
environment (the `Runtime` class) - and forms the argument
to the thread function. The `thread$create` call returns a handle to the
new thread. In your main thread, wait for threads to complete by
passing a list of handles to `thread$join`. If a thread completes
and leaves any items on its stack, the top item is copied into its
*retval*. Once the thread has finished (typically after a join) this
can be read from the handle with `thread$retval`.

Thus we can write code like this:
```
`time library drop

(
    []
    0 10 range each {
        i (|a:| 
            0.1 time$delay
            ?a dup *
          ) thread$create,
    }
    dup thread$join (thread$retval) map
    show.
)@
quit
```
which will start a separate thread for each number 0..9, calculating
and returning their squares after a bit of a delay. The threads will
all be joined, and the return values obtained and printed.

## Making it work
My first attempt at fixing this was bottom-up: add rwlocks to the 
underlying structures and work upwards, dealing with the consequences.
Unfortunately the consequences were very messy indeed. This aborted
work still exists in the branch `badthreads`. Don't use it except
for reference.

It might be better to work top-down: deal with the separate cases
where these dangerous structures are used, first tidying up the
API so that no dangling pointers are leaked out, and secondly
adding locks around the high-level structures. Care must be taken
to deal with exceptions - either removing exception handling code
and propagating badness upwards through the stack (perhaps using
some kind of `Maybe` analogue), or by adding exception handlers
all the way up. RAAI might be the right way to do it, but I'd like
to use plain POSIX threads. Old school. Maybe I should just abandon
those "principles".

Anyway, so the method for each "thing" is:
- modify API to remove pointer/reference returns to movable things (making things higher-level if required)
- add locks around the API, using rwlocks or shared_mutex in C++11
- make sure exceptions don't leave things locked

And the "things" are (as far as I can tell)
###Done things
- the symbol table - DONE - `tSymbol` (the type object) is lockable.
- the namespace data - DONE - the name manager is lockable.
- `ListObject` arrays - DONE - `ArrayList` is lockable, so `ListObject` wraps it with calls which lock
- list iterators create a writelock on the list, which is destroyed when the iterator completes or is destroyed. Resetting with `first should be OK too.
- list cloning is inside writelock on the destination list, and the iterator will create a readlock on the source list.
- global locking (unfortunately) on increment/decrement operations on gc objects.
###Not done things
- test removing the GC global locks
- `HashObject` hashes and their iterators

