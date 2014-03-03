/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#include "angort.h"
#include "ser.h"

#include <time.h>

%name std


%word version ( -- version ) version number
{
    a->pushInt(a->getVersion());
}

%word barewords (v --) turn bare words on or off
{
    a->barewords = a->popInt()?true:false;
}

%word dump ( title -- ) dump the stack
{
    char buf[1024];
    const char *s = a->popval()->toString(buf,1024);
    a->dumpStack(s);
}

%word snark ( ) used during debugging; prints an autoincremented count
{
    static int snarkct=0;
    printf("SNARK %d\n",snarkct++);
}


%word p ( v -- ) print a value
{
    char buf[1024];
    Value *v = a->popval();
    fputs(v->toString(buf,1024),stdout);
}

%word x (v -- ) print a value in hex
{
    int x = a->popval()->toInt();
    printf("%x",x);
}

%word rawp (v --) print a ptr value as raw hex
{
    void *p = a->popval()->getRaw();
    printf("%p",p);
}


%word nl ( -- ) print a new line
{
    puts("");
}

%word quit ( -- ) exit with return code 0
{
    exit(0);
}

%word debug ( v -- ) turn debugging on or off (value is 0 or 1)
{
    a->debug = a->popInt()?true:false;
}

%word disasm (name -- ) disassemble word
{
    char buf[1024];
    const char *name = a->popval()->toString(buf,1024);
    a->disasm(name);
}


/// this exception is thrown when a generic runtime error occurs

class AssertException : public Exception {
public:
    AssertException(const char *desc,int line){
        snprintf(error,1024,"Assertion failed at line %d:  %s",line,desc);
    }
};

%word assertdebug (bool --) turn assertion printout on/off
{
    a->assertDebug = a->popInt()!=0;
}

%word assert (bool desc --) throw exception with string 'desc' if bool is false
{
    char buf[1024];
    const char *desc = a->popval()->toString(buf,1024);
    if(!a->popInt()){
        if(a->assertDebug)printf("Assertion failed: %s\n",desc);
        throw AssertException(desc,a->getLineNumber());
    } else if(a->assertDebug)
        printf("Assertion passed: %s\n",desc);
        
}

%word abs (x --) absolute value
{
    Value *v = a->popval();
    if(v->t == Types::tInteger){
        int i;
        i = v->toInt();
        a->pushInt(i<0?-i:i);
    } else if(v->t == Types::tFloat){
        float f;
        f = v->toFloat();
        a->pushFloat(f<0?-f:f);
    } else {
        throw RUNT("bad type for 'abs'");
    }
}

%word neg (x --) negate
{
    Value *v = a->popval();
    if(v->t == Types::tInteger){
        int i;
        i = v->toInt();
        a->pushInt(-i);
    } else if(v->t == Types::tFloat){
        float f;
        f = v->toFloat();
        a->pushFloat(-f);
    } else {
        throw RUNT("bad type for 'neg'");
    }
}
    

%word isnone (val -- bool) is a value NONE
{
    Value *s = a->stack.peekptr();
    Types::tInteger->set(s,s->isNone()?1:0);
}

%word gccount (-- val) return the number of GC objects
{
    a->pushInt(GarbageCollected::getGlobalCount());
}

%word range (start end -- range) build a range object
{
    int end = a->popInt();
    int start = a->popInt();
    Value *v = a->pushval();
    Types::tIRange->set(v,start,end,end>=start?1:-1);
}

%word srange (start end step -- range) build a range object with step
{
    int step = a->popInt();
    int end = a->popInt();
    int start = a->popInt();
    Value *v = a->pushval();
    Types::tIRange->set(v,start,end,step);
}

%word frange (start end step -- range) build a float range object with step size
{
    float step = a->popFloat();
    float end = a->popFloat();
    float start = a->popFloat();
    Value *v = a->pushval();
    Types::tFRange->set(v,start,end,step);
}

%word frangesteps (start end stepcount -- range) build a float range object with step count
{
    float steps = a->popFloat();
    float end = a->popFloat();
    float start = a->popFloat();
    float step = (end-start)/steps;
    end += step * 0.4f;
    
    Value *v = a->pushval();
    Types::tFRange->set(v,start,end,step);
}

%word i (-- current) get current iterator value
{
    Value *p = a->pushval();
    p->copy(a->getTopIterator()->current);
}
%word j (-- current) get nested iterator value
{
    Value *p = a->pushval();
    p->copy(a->getTopIterator(1)->current);
}
%word k (-- current) get nested iterator value
{
    Value *p = a->pushval();
    p->copy(a->getTopIterator(2)->current);
}

%word iter (-- iterable) get the iterable which is currently being looped over
{
    Value *p = a->pushval();
    IteratorObject *iterator = a->getTopIterator();
    p->copy(iterator->iterable);
}
    

%word save (name --) Hopefully save an image
{
    char b[1024];
    const char *name = a->popval()->toString(b,1024);
    Serialiser ser;
    
    ser.save(a,name);
}
%word load (name --) Hopefully load an image
{
    char b[1024];
    const char *name = a->popval()->toString(b,1024);
    Serialiser ser;
    
    ser.load(a,name);
}

%word reset (--) Clear everything
{
    while(!a->stack.isempty()){
        Value *v = a->popval();
        v->clr();
    }
    a->clear();
}

%word clear (--) Clear the stack, and also set all values to None
{
    while(!a->stack.isempty()){
        Value *v = a->popval();
        v->clr();
    }
}

%word list (--) List everything
{
    a->list();
}

%word help (s --) get help on a word or native function
{
    char b[1024];
    const char *name = a->popval()->toString(b,1024);
    const char *s = a->getSpec(name);
    if(!s)s="no help found";
    printf("%s: %s\n",name,s);
}


%word type (v --) get the type of the item as a string
{
    Value *v = a->popval();
    const char *name = v->t->name;
    
    v = a->pushval();
    Types::tString->set(v,name);
}

%word srand (i --) set the random number generator seed. If -1, use the timestamp.
{
    int v = a->popInt();
    if(v==-1){
        long t;
        time(&t);
        srand(t);
    } else 
        srand(v);
}


%word rand (-- i) stack an integer random number
{
    a->pushInt(rand());
}


