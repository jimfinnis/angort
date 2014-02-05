/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#include "angort.h"
#include "ser.h"

%name std


%word version ( -- version ) version number
{
    a->pushInt(ANGORT_VERSION);
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

%word isnone (val -- bool) is a value NONE
{
    Value *s = a->stack.peekptr();
    Types::tInteger->set(s,s->isNone()?1:0);
}

%word gccount (-- val) return the number of GC objects
{
    a->pushInt(GarbageCollected::getGlobalCount());
}

%word range (start end step -- range) TEST build a range object
{
    int step = a->popInt();
    int end = a->popInt();
    int start = a->popInt();
    Value *v = a->pushval();
    Types::tRange->set(v,start,end,step);
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
    

%word iter (iterable -- iterator) TEST word for creating an iterator
{
    Value *v = a->stack.peekptr();
    v->t->createValueIterator(v,v);
}


%word save (name --) Hopefully save an image
{
    char name[1024];
    a->popval()->toString(name,1024);
    Serialiser ser;
    
    ser.save(a,name);
}
%word load (name --) Hopefully load an image
{
    char name[1024];
    a->popval()->toString(name,1024);
    Serialiser ser;
    
    ser.load(a,name);
}

%word reset (--) Clear everything
{
    a->clear();
}

%word list (--) List everything
{
    a->list();
}


%word type (v --) get the type of the item as a string
{
    Value *v = a->popval();
    const char *name = v->t->name;
    
    v = a->pushval();
    Types::tString->set(v,name);
}
