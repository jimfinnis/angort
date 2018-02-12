/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#include "angort.h"
#include "cycle.h"

#include <signal.h>
#include <unistd.h>
#include <time.h>

using namespace angort;

void sighandler(int sig){
    signal(sig,NULL);
    char buf[32];
    sprintf(buf,"ex$sig%d",sig);
    throw RUNT(buf,"").set("Caught signal %d (%s)",sig,strsignal(sig));
}

namespace angort {

/// define a property to set and get the auto cycle detection interval.
/// It will be called "autogc".

class AutoGCProperty: public Property {
private:
    Runtime *run;
public:
    AutoGCProperty(Runtime *_a){
        run = _a;
    }
    
    virtual void postSet(){
        run->ang->globalLock();
        run->ang->autoCycleInterval = v.toInt();
        run->autoCycleCount = run->ang->autoCycleInterval;
        run->ang->globalUnlock();
    }
    
    virtual void preGet(){
        Types::tInteger->set(&v,run->ang->autoCycleInterval);
    }
};

/// a property to get and set the library search path,
/// called "searchpath".
class SearchPathProperty : public Property {
private:
    Angort *a;
public:
    SearchPathProperty(Runtime *_a){
        a = _a->ang;
    }
    
    virtual void postSet(){
        if(a->searchPath)
            free((void *)a->searchPath);
        a->searchPath = strdup(v.toString().get());
    }
    
    virtual void preGet(){
        Types::tString->set(&v,
                            a->searchPath ? a->searchPath:DEFAULTSEARCHPATH);
    }
};

// assumes (nsid name --) on the stack
static NamespaceEnt *getNSEnt(Runtime *a){
    const StringBuffer &s = a->popString();
    int nsid = Types::tNSID->get(a->popval());
    
    Namespace *ns = a->ang->names.getSpaceByIdx(nsid);
    
    int idx = ns->get(s.get());
    if(idx<0)
        throw RUNT(EX_NOTFOUND,"cannot find name in namespace");
    return ns->getEnt(idx);
}

}// end namespace




%name std

static void trapsig(int sig){
    struct sigaction act;
    
    act.sa_handler = sighandler;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask,SIGABRT);
    act.sa_flags = 0;
    sigaction(sig,&act,NULL);
}

%wordargs signal i (sig --) install a signal handler for signal number sig
After this call, signal number i will result in an exception ex$sigN,
where N is the signal number i.
{
    trapsig(p0);
}

%word signals (--) install a handler for a good subset of signals
After this call, a signal will result in an exception ex$sigN,
where N is the signal number. The signals trapped are:
SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGTRAP, SIGIOT, SIGBUS, SIGFPE, SIGUSR1,
SIGSEGV SIGUSR2, SIGPIPE, SIGALRM, SIGTERM, SIGALRM, SIGSTKFLT, SIGCHLD,
SIGCONT, SIGTSTP SIGTTIN, SIGTTOU, SIGURG, SIGXCPU, SIGXFSZ, SIGVTALRM,
SIGPROF, SIGWINCH, SIGIO SIGPWR, SIGSYS
{
    trapsig(SIGHUP);
    trapsig(SIGINT);
    trapsig(SIGQUIT);
    trapsig(SIGILL);
    trapsig(SIGTRAP);
    trapsig(SIGIOT);
    trapsig(SIGBUS);
    trapsig(SIGFPE);
    trapsig(SIGUSR1);
    trapsig(SIGSEGV);
    trapsig(SIGUSR2);
    trapsig(SIGPIPE);
    trapsig(SIGALRM);
    trapsig(SIGTERM);
    trapsig(SIGALRM);
    trapsig(SIGSTKFLT);
    trapsig(SIGCHLD);
    trapsig(SIGCONT);
    trapsig(SIGTSTP);
    trapsig(SIGTTIN);
    trapsig(SIGTTOU);
    trapsig(SIGURG);
    trapsig(SIGXCPU);
    trapsig(SIGXFSZ);
    trapsig(SIGVTALRM);
    trapsig(SIGPROF);
    trapsig(SIGWINCH);
    trapsig(SIGIO);
    trapsig(SIGPWR);
    trapsig(SIGSYS);
}


%wordargs strsignal i (sig -- name) get signal name for signal number
Return the standard signal name for a given signal, e.g. "Hangup" for
SIGHUP (1).
{
    a->pushString(strsignal(p0));
}



%word version ( -- version ) version number
Return the Angort version number as an string.
{
    a->pushString(a->ang->getVersion());
}

%word barewords (v --) turn bare words on or off
If set to true, unknown identifiers will be converted to symbol values.
{
    a->ang->barewords = a->popInt()?true:false;
}

%wordargs show v (v -- s) show a variable into a string
This shows more detail than a . or p, for example recursively showing
lists and hashes. It outputs to a string.
{
    char *p=NULL;
    p0->dump(&p);
    a->pushString(p);
    free(p);
}


%word dump ( title -- ) dump the stack
Print the values on the stack with a title, using naive string conversion
(e.g. all the items in a list will not be printed, just a brief string
identifying the list).
{
    a->dumpStack(a->popString().get());
}

%wordargs raisesignal i (sig --) throw a signal
{
    kill(getpid(),p0);
}

%word snark ( ) used during debugging; prints an autoincremented count
Simply prints SNARK N, where N increments at every call. These are added
to programs during debugging, so that one can "hunt the snarks" when
removing them.
{
    static int snarkct=0;
    printf("SNARK %d\n",snarkct++);
}

%word none ( -- none ) stack a None value
{
    Value *v = a->pushval();
    v->clr();
}

%word ct (-- ct) push the current stack count
{
    a->pushInt(a->stack.ct);
}

%word rct (gcv -- ct) push the ref count for a GC value
For a garbage collected item, push the reference count. Pushes 0
for other items.
{
    Value *v = a->popval();
    GarbageCollected *gc = v->t->getGC(v);
    if(gc){
        a->pushInt(gc->refct);
    } else 
        a->pushInt(0);
}

%word p ( v -- ) print a value
Prints a string representation of a value to stdout without a
trailing newline.
{
    fputs(a->popString().get(),a->outputStream);
}

%word x (v -- ) print a value in hex
Prints a hex representation of an integer value to stdout without a
trailing newline.
{
    int x = a->popval()->toInt();
    fprintf(a->outputStream,"%x",x);
}

%word rawp (v --) print a ptr value as raw hex
Prints a raw hex representation of an  value to stdout without a
trailing newline.
{
    void *p = a->popval()->getRaw();
    fprintf(a->outputStream,"%p",p);
}


%wordargs redir s (filename --) open a new file and redirect output to it.
For more complex file output, use the IO library. On fail, throws
ex$failed.
{
    a->endredir();
    FILE *f = fopen(p0,"w");
     if(!f)
        throw RUNT(EX_FAILED,"").set("redir unable to open file %s",p0);
    a->outputStream=f;
}

%word endredir (--) close a redirected output stream
{
    a->endredir();
}


%word nl ( -- ) print a new line
{
    fputc('\n',a->outputStream);
}

%word errp ( s -- ) print a string to stdout
{
    const StringBuffer& str = a->popString();
    fputs(str.get(),stderr);
}

%word errnl ( -- ) print newline to stdout
{
    fputc('\n',stderr);
}


%word quit ( -- ) exit with return code 0
Typically used to terminate an Angort program, which would normally
drop back to the interpreter.
{
    //todo threads - might cause problems if threads survive
    a->ang->shutdown();
    exit(0);
}

%word abort ( s -- ) exit with return code 1 and an error message to stderr
{
    //todo threads - might cause problems if threads survive
    a->ang->shutdown();
    fprintf(stderr,"%s\n",a->popString().get());
    exit(1);
}

%word debug ( v -- ) turn debugging on or off (value is 0 or 1)
If true, every instruction executed will print to stdout along
with a stack dump. Slows down the program!
{
    a->trace = a->popInt()!=0;
}

%word disasm (name -- ) disassemble word
Print to stdout the instructions for a named Angort function.
{
    //todo threads - might cause weirdness if invoked in another thread, so
    //turning it off there
    a->checkzerothread();
    a->ang->disasm(a->popString().get());
}

%word assertdebug (bool --) turn assertion printout on/off
Assertions will not be printed if this is false. If true,
both true and false assertions will print.
{
    a->assertDebug = a->popBool()!=0;
}

%word assert (bool desc --) throw exception with string 'desc' if bool is false
Asserts that the boolean is true, throwing ex$assert if not. Will
print the assertion, even if it passes, if assertdebug has been set.
If assertmode has been set to `negated, false assertions will pass (used
in assertion testing). All prints are to stdout.
{
    const StringBuffer &desc = a->popString();
    bool cond = (a->popBool()==0);
    
    if(a->assertNegated){
        cond=!cond;
        if(a->assertDebug)printf("Negated ");
    }
    if(cond){
        if(a->assertDebug)printf("Assertion failed: %s\n",desc.get());
        throw AssertException(desc.get(),a->ang->getLineNumber());
    } else if(a->assertDebug)
        printf("Assertion passed: %s\n",desc.get());
}

static int stackcheck=-1;
%word chkstart (--) start stack check block (save stack ct)
Records the stack count, ready for "chkend".
{
    a->checkzerothread();
    stackcheck = a->stack.ct;
}
%word chkend (--) end stack check block (check stack count agrees with saved)
If the current stack count does not match that stored by the last
"chkstart", throw an ex$assert exception.
{
    if(a->stack.ct!=stackcheck)
        throw AssertException("stack check failed",a->ang->getLineNumber());
    
}




%word assertmode (mode --) set to `negated or `normal, if negated assertion conditions are negated
If set to `negated, subsequent assertions pass if they are false. This is
used to test the assertion functionality.
{
    const StringBuffer& sb = a->popString();
    if(!strcmp(sb.get(),"negated"))
        a->assertNegated=true;
    else
        a->assertNegated=false;
}

%word asserteq (a b -- ) if a!=b assert
Shortcut for asserting that two integer values are equal.
{
    if(a->popInt()!=a->popInt())
        throw RUNT(EX_ASSERT,"assertq failure");
}

%word abs (x --) absolute value
return the absolute value of a float or int.
{
    Value *v = a->stack.peekptr();
    v->t->absolute(v,v);
}

%word neg (x --) negate
Negate an int or float.
{
    Value *v = a->stack.peekptr();
    v->t->negate(v,v);
}
    

%word isnone (val -- bool) is a value NONE
Returns true if the value is a NONE value.
{
    Value *s = a->stack.peekptr();
    Types::tInteger->set(s,s->isNone()?1:0);
}

%word isnumber (val -- bool) return true if a numeric type
{
    Value *s = a->stack.peekptr();
    Types::tInteger->set(s,s->t->flags & TF_NUMBER?1:0);
}

%word iscallable (val -- bool) return true if is codeblock or closure
Return true if the value is a function.
{
    Value *s = a->stack.peekptr();
    Types::tInteger->set(s,s->t->isCallable()?1:0);
}

%word gccount (-- val) return the number of GC objects
Returns the total number of garbage collected objects in the system.
{
    a->pushInt(GarbageCollected::getGlobalCount());
}

%wordargs getglobal s (string -- val) get a global by name
Gets the value of a global variable by name.
{
    int id = a->ang->findOrCreateGlobal(p0);
    Value *v = a->ang->names.getVal(id);
    // have to deal with properties too. Ugly.
    if(v->t == Types::tProp){
        v->v.property->preGet();
        a->pushval()->copy(&v->v.property->v);
        v->v.property->postGet();
    }
    else
        a->pushval()->copy(a->ang->names.getVal(id));
}
%wordargs setglobal vs (val string -- val) set a global by name
Sets the value of a global variable by name.
{
    int id = a->ang->findOrCreateGlobal(p1);
    Value *v = a->ang->names.getVal(id);
    // have to deal with properties too. Ugly.
    if(v->t == Types::tProp){
        v->v.property->preSet();
        v->v.property->v.copy(p0);
        v->v.property->postSet();
    }
    else
        a->ang->names.getVal(id)->copy(p0);
}


%word range (start end -- range) build a range object
Returns a new integer range object, which covers the range [start,end),
i.e. "0 10 range" returns a range for the values 0 to 9. If the end
is less than the start, the range will run backwards (step will be -1),
otherwise the step will be 1.
{
    int end = a->popInt();
    int start = a->popInt();
    Value *v = a->pushval();
    Types::tIRange->set(v,start,end,end>=start?1:-1);
}

%word srange (start end step -- range) build a range object with step
Returns a new integer range object, which covers the range [start,end),
i.e. "0 10 1 srange" returns a range for the values 0 to 9. The step
gives the interval, so "0 10 2 srange " will give the numbers 0,2,4,6,8.
{
    int step = a->popInt();
    int end = a->popInt();
    int start = a->popInt();
    Value *v = a->pushval();
    Types::tIRange->set(v,start,end,step);
}

%word frange (start end step -- range) build a float range object with step size
Returns a new floating point range object, with a step size for iteration.
This excludes the end value, so "0 10 1 frange" will iterate over 0-9.
{
    float step = a->popFloat();
    float end = a->popFloat();
    float start = a->popFloat();
    Value *v = a->pushval();
    Types::tFRange->set(v,start,end,step);
}

%word frangesteps (start end stepcount -- range) build a float range object with step count
Returns a new floating point range object, generating the step size from
the number of steps required. This excludes the end value, so
"0 10 10 frangesteps" will iterate over 0-9.
{
    float steps = a->popFloat();
    float end = a->popFloat();
    float start = a->popFloat();
    float step = (end-start)/steps;
    end += step * 0.4f;
    
    Value *v = a->pushval();
    Types::tFRange->set(v,start,end,step);
}

%word i (-- current) get current iterator value (key if hash)
If in an iterator ("each") loop, return the current value. For hashes,
this will return the key.
{
    Value *p = a->pushval();
    p->copy(a->getTopIterator()->current);
}
%word j (-- current) get nested iterator value (key if hash)
If in an 2-deep iterator ("each") loop, return the current value of
the outer loop. For hashes, this will return the key.
{
    Value *p = a->pushval();
    p->copy(a->getTopIterator(1)->current);
}
%word k (-- current) get nested iterator value (key if hash)
If in an 3-deep iterator ("each") loop, return the current value of
the outer loop. For hashes, this will return the key.
{
    Value *p = a->pushval();
    p->copy(a->getTopIterator(2)->current);
}

%word ival (-- current) get current iterator value (value if hash)
If in an iterator ("each") loop, return the current value. For hashes,
this will return the value (as opposed to "i").
{
    Value *iterable = a->getTopIterator()->iterable;
    Value *cur = a->getTopIterator()->current;
    Value *p = a->pushval();
    iterable->t->getValue(iterable,cur,p);
}

%word jval (-- current) get nested iterator value (value if hash)
If in an 2-deep iterator ("each") loop, return the current value of
the outer loop. For hashes, this will return the value (as opposed to "j").
{
    Value *iterable = a->getTopIterator(1)->iterable;
    Value *cur = a->getTopIterator(1)->current;
    Value *p = a->pushval();
    iterable->t->getValue(iterable,cur,p);
}

%word kval (-- current) get nested iterator value (value if hash)
If in an 3-deep iterator ("each") loop, return the current value of
the outer loop. For hashes, this will return the value (as opposed to "k").
{
    Value *iterable = a->getTopIterator(2)->iterable;
    Value *cur = a->getTopIterator(2)->current;
    Value *p = a->pushval();
    iterable->t->getValue(iterable,cur,p);
}

%word iidx (-- index) get current iterator index integer
The iterator index counts how many times the iterator has moved forwards
in the iterable.
{
    // this subtracts 1 because OP_ITERLEAVEIFDONE advances the
    // iterator at the start of the loop code, after stashing the
    // current value away.
    a->pushInt(a->getTopIterator()->iterator->index()-1);
}    

%word jidx (-- index) get nested iterator index integer.
The iterator index counts how many times the iterator has moved forwards
in the iterable.
{
    // this subtracts 1 because OP_ITERLEAVEIFDONE advances the
    // iterator at the start of the loop code, after stashing the
    // current value away.
    a->pushInt(a->getTopIterator(1)->iterator->index()-1);
}    

%word kidx (-- index) get 2nd nested iterator index integer.
The iterator index counts how many times the iterator has moved forwards
in the iterable.
{
    // this subtracts 1 because OP_ITERLEAVEIFDONE advances the
    // iterator at the start of the loop code, after stashing the
    // current value away.
    a->pushInt(a->getTopIterator(2)->iterator->index()-1);
}    


%word iter (-- iterable) get the iterable which is currently being looped over
Return the actual iterable object which is being iterated over inside an
"each" loop.
{
    Value *p = a->pushval();
    IteratorObject *iterator = a->getTopIterator();
    p->copy(iterator->iterable);
}

%word mkiter (iterable -- iterator) make an iterator object
Create an iterator for an iterable value. Typical usage might be
something like 
 "?L mkiter !I { ?I idone ifleave ?I icurnext.}"
although "each" is advisable for this simple situation.
{
    Value *p = a->stack.peekptr();
    p->t->createIterator(p,p);
}

%word icur (iterator -- item) get current item in iterator made with mkiter
Get the current value of an iterator created with "mkiter". Returns
NONE if the iterator has completed.
{
    Value *p = a->stack.peekptr();
    Iterator<Value *>* i = Types::tIter->get(p);
    if(i->isDone())
        p->clr();
    else
        p->copy(i->current());
}

%word inext (iterator --) advance the iterator
Advance an iterator created with "mkiter".
{
    Value *p = a->stack.popptr();
    Iterator<Value *>* i = Types::tIter->get(p);
    i->next();
}

%word icurnext (iterator -- item) combined icur and inext
Get the current value of an iterator created with "mkiter", advance
the iterator and return the value. Returns NONE if the iterator has
completed.
{
    Value *p = a->stack.peekptr();
    Iterator<Value *>* i = Types::tIter->get(p);
    if(i->isDone())
        p->clr();
    else {
        p->copy(i->current());
        i->next();
    }
        
}

%word ifirst (iterator --) reset the iterator to the start
Reset an iterator created with "mkiter" to the start of the iterable.
{
    Value *p = a->stack.popptr();
    Iterator<Value *>* i = Types::tIter->get(p);
    i->first();
}

%word idone (iterator -- boolean) true if iterator is done
Return true if an iterator created with "mkiter" has completed.
{
    Value *p = a->stack.peekptr();
    Iterator<Value *>* i = Types::tIter->get(p);
    Types::tInteger->set(p,i->isDone()?1:0);
}
    

%word reset (--) Clear everything
Clears the stack and resets all namespaces.
{
    while(!a->stack.isempty()){
        Value *v = a->popval();
        v->clr();
    }
    a->clear();
}

%word clear (--) Clear the stack, and also set all values to None
Clears the stack and resets all stack values to NONE, which should
release any stale garbage collected objects which were previously
on the stack (although those in the local variable stack and in
globals will survive).
{
    while(!a->stack.isempty()){
        Value *v = a->popval();
        v->clr();
    }
}

%word list (--) List everything
List the names registered in all namespaces.
{
    a->ang->list();
}

%word help (s --) get help on a word or native function
This is the same as "??name".
{
    const StringBuffer &name = a->popString();
    const char *s = a->ang->getSpec(name.get());
    if(!s)s="no help found";
    printf("%s: %s\n",name.get(),s);
}

%word listhelp (s --) list and show help for all public functions in the given namespace
List all words in a given namespace, showing the help texts for all
of them.
{
    Namespace *s = a->ang->names.getSpaceByName(a->popString().get());
    for(int i=0;i<s->count();i++){
        NamespaceEnt *e = s->getEnt(i);
        if(!e->isPriv){
            const char *spec = e->spec?e->spec:"";
            if(spec)
                printf("%-20s : %s\n",s->getName(i),spec);
            else
                printf("%-20s\n",s->getName(i));
        }
    }                                  
}


%word type (v -- symb) get the type of the item as a symbol
{
    Value *v = a->stack.peekptr();
    Types::tSymbol->set(v,v->t->nameSymb);
}

%word listtypes ( -- ) print the types
Lists the names of all Angort types, including the internal ones
(such as "natprop" and "deleted").
{
    Type::dumpTypes();
}


%word gc (--) perform a major garbage detect and cycle removal
Runs a major garbage detect, which includes detecting cycles. This
is automatically run every "autogc" ticks, so you may need to do this
if your program has done "0!autogc" to turn that off. Alternatively
you may not if your program does not create cyclic references (data
structures which refer to themselves).
{
    a->gc();
}

%word nspace (name -- nsid) get a namespace by name
Return a namespace ID for a given namespace. Throws ex$notfound if there
is no such namespace.
{
    const StringBuffer& name = a->popString();
    Namespace *ns = a->ang->names.getSpaceByName(name.get());
    Types::tNSID->set(a->pushval(),ns->idx);
}

%word nspaces (-- list) return a list of all namespaces in index order
Returns a list names of all namespaces, in the order in which they
were created.
{
    ArrayList<Value> *list=Types::tList->set(a->pushval());
    a->ang->names.spaces.appendNamesToList(list);
}

%word names (nsid --) get a list of names from a namespace
Returns a list of names from a namespace whose handle has been obtained
with "nspace".
{
    int idx = Types::tNSID->get(a->popval());
    Namespace *ns = a->ang->names.getSpaceByIdx(idx);
    if(!ns)a->pushNone();
    else {
        ArrayList<Value> *list=Types::tList->set(a->pushval());
        ns->appendNamesToList(list);
    }
}


%word ispriv (nsid name -- bool) return true if the definition is private in the namespace
Returns true if the identifier is private inside the given namespace, which
is identified by a nsid returned by "nspace".
{
    NamespaceEnt *ent = getNSEnt(a);
    a->pushInt(ent->isPriv?1:0);
    
}
%word isconst (nsid name -- bool) return true if the definition is constant in the namespace
Returns true if the identifier is modifiable inside the given namespace, which
is identified by a nsid returned by "nspace".
{
    NamespaceEnt *ent = getNSEnt(a);
    a->pushInt(ent->isConst?1:0);
    
}

%word lookup (nsid name -- value) look up the value of a namespace entity
Looks up an identifier in a namespace, which is identified by a nsid
returned by "nspace", and returns its value.
{
    NamespaceEnt *ent = getNSEnt(a);
    a->pushval()->copy(&ent->v);
}
    
%word tostr (val -- string) convert value to string
{
    const StringBuffer& str = a->popString();
    
    // we'll get a problems if we try to do this is
    // one move, because we may be writing a string back
    // to itself. This will deallocate the string before
    // it can be copied over onto itself.
    Value tmp;
    Types::tString->set(&tmp,str.get());
    a->pushval()->copy(&tmp);
    
}
    

%word tosymbol (val -- symbol) convert value to symbol
{
    const StringBuffer& name = a->popString();
    int symb = SymbolType::getSymbol(name.get());
    
    Types::tSymbol->set(a->pushval(),symb);
    
}

%word tolong (val -- long) -- convert value to long int
{
    Types::tLong->set(a->pushval(),a->popval()->toLong());
}

%word todouble (val -- double) -- convert value to double
{
    Types::tDouble->set(a->pushval(),a->popval()->toDouble());
}


%word endpackage (-- namespaceID) mark end of package, only when used within a single script
For packages which are part of a long script, this marks the end. Normally
the end of a package is marked by the end of the file.
{
    a->checkzerothread();
    a->ang->endPackageInScript();
}


%word dumpframe () debugging - dump the frame variables and GC objects
Prints internal debugging data.
{
    a->dumpFrame();
}

%word bor (a b -- a|b) bitwise or
{
    Value *p[2];
    a->popParams(p,"nn");
    int x = p[0]->toLong();
    int y = p[1]->toLong();
    a->pushLong(x|y);
}
%word band (a b -- a&b) bitwise and
{
    Value *p[2];
    a->popParams(p,"nn");
    int x = p[0]->toLong();
    int y = p[1]->toLong();
    a->pushLong(x&y);
}
%word bxor (a b -- a^b) bitwise xor
{
    Value *p[2];
    a->popParams(p,"nn");
    int x = p[0]->toLong();
    int y = p[1]->toLong();
    a->pushLong(x^y);
}
%word bnot (a -- ~a) bitwise not
{
    Value *p;
    a->popParams(&p,"n");
    int x = p->toLong();
    a->pushLong(~x);
}
%word shiftleft (a b -- a<<b) bitshift left
{
    Value *p[2];
    a->popParams(p,"nn");
    int x = p[0]->toLong();
    int y = p[1]->toLong();
    a->pushLong(x<<y);
}
%word shiftright (a b -- a>>b) bitshift right
{
    Value *p[2];
    a->popParams(p,"nn");
    int x = p[0]->toLong();
    int y = p[1]->toLong();
    a->pushLong(x>>y);
}

%word read (-- s|none) read line from stdin
Reads and returns a line from stdin, unless the end of file has been
reached in which case NONE is returned.
{
    if(!feof(stdin)){
        char *buf=NULL;
        size_t size;
        int rv = getline(&buf,&size,stdin);
        if(rv<=0)
            a->pushNone();
        else{
            buf[rv-1]=0;
            a->pushString(buf);
        }
        if(buf)free(buf);
    } else
        a->pushNone();
}

%wordargs setfloatformat s (s --) set a printf format string for floats
Sets the format string used for the standard conversion from floats
to strings. The default is "%f". Doubles have a separate string set
with setdoubleformat.
{
    if(strlen(p0)>63)
        throw RUNT(EX_LONGNAME,"format string too long");
    strcpy(Types::tFloat->formatString,p0);
}

%wordargs setdoubleformat s (s --) set a printf format string for doubless
Sets the format string used for the standard conversion from doubles
to strings. The default is "%f". Floats have a separate string set
with setfloatformat.
{
    if(strlen(p0)>63)
        throw RUNT(EX_LONGNAME,"format string too long");
    strcpy(Types::tDouble->formatString,p0);
}

%wordargs library s (name -- nsid) load a plugin library, returning the namespace ID
This loads a C++ native library and returns the NSID. Typically the
next step will be to either drop or import the library. The latter
will be to mark the library's namespace as imported, so that its
symbols can be used without a fully-qualified name.
{
    
    Types::tNSID->set(a->pushval(),a->ang->plugin(p0));
}

%word import (namespaceint --) or (namespaceint list --) import namespace or namespace members into default space
The first form will add a namespace to the "imported namespace list",
so that all members of the namespace will be available without full
qualification (i.e. without a "$").
The second form will import copy single values from a namespace
into the default namespace, making them available without full
qualification. The idiom for this is typically:
`libname nspace [`s1,`s2..] import
{
    a->checkzerothread();
    if(a->stack.peekptr()->t==Types::tNSID){
        int nsid = Types::tNSID->get(a->popval());
        a->ang->names.import(nsid,NULL);
    } else if(a->stack.peekptr()->t==Types::tList){
        ArrayList<Value> *lst = Types::tList->get(a->popval());
        int nsid = Types::tNSID->get(a->popval());
        a->ang->names.import(nsid,lst);
    } else
        throw SyntaxException("expected package list or package in import");
}


/*%word showclosure (cl --)
{
    Value *v = a->popval();
    if(v->t == Types::tClosure)
        v->v.closure->show("Show command");
}*/

%shared
%init
{
    a->ang->registerProperty("autogc",new angort::AutoGCProperty(a));
    a->ang->registerProperty("searchpath",new angort::SearchPathProperty(a));
}
    
