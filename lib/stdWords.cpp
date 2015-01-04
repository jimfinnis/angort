/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#include "angort.h"
#include "cycle.h"

#include <time.h>

using namespace angort;


namespace angort {

/// define a property to set and get the auto cycle detection interval.
/// It will be called "autogc".

class AutoGCProperty: public Property {
private:
    Angort *a;
public:
    AutoGCProperty(Angort *_a){
        a = _a;
    }
    
    virtual void postSet(){
        a->autoCycleInterval = v.toInt();
        a->autoCycleCount = a->autoCycleInterval;
    }
    
    virtual void preGet(){
        Types::tInteger->set(&v,a->autoCycleInterval);
    }
};

/// a property to get and set the library search path,
/// called "searchpath".
class SearchPathProperty : public Property {
private:
    Angort *a;
public:
    SearchPathProperty(Angort *_a){
        a = _a;
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

static NamespaceEnt *getNSEnt(Angort *a){
    const StringBuffer &s = a->popString();
    Namespace *ns = a->names.getSpaceByIdx(a->popInt());
    
    int idx = ns->get(s.get());
    if(idx<0)
        throw RUNT("ispriv: cannot find name in namespace");
    return ns->getEnt(idx);
}
}// end namespace

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
    a->dumpStack(a->popString().get());
}

%word snark ( ) used during debugging; prints an autoincremented count
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

%word rct (gcv -- ct) push the GC count for a GC value
{
    Value *v = a->popval();
    GarbageCollected *gc = v->t->getGC(v);
    if(gc){
        a->pushInt(gc->refct);
    }
}

%word p ( v -- ) print a value
{
    fputs(a->popString().get(),stdout);
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
    a->shutdown();
    exit(0);
}

%word debug ( v -- ) turn debugging on or off (value is 0 or 1)
{
    a->debug = a->popInt();
}

%word disasm (name -- ) disassemble word
{
    a->disasm(a->popString().get());
}

%word assertdebug (bool --) turn assertion printout on/off
{
    a->assertDebug = a->popInt()!=0;
}

%word assert (bool desc --) throw exception with string 'desc' if bool is false
{
    const StringBuffer &desc = a->popString();
    bool cond = (a->popInt()==0);
    
    if(a->assertNegated){
        cond=!cond;
        if(a->assertDebug)printf("Negated ");
    }
    if(cond){
        if(a->assertDebug)printf("Assertion failed: %s\n",desc.get());
        throw AssertException(desc.get(),a->getLineNumber());
    } else if(a->assertDebug)
        printf("Assertion passed: %s\n",desc.get());
}

%word assertmode (mode --) set to `negated or `normal, if negated assertion conditions are negated
{
    const StringBuffer& sb = a->popString();
    if(!strcmp(sb.get(),"negated"))
        a->assertNegated=true;
    else
        a->assertNegated=false;
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

%word iscallable (val -- bool) return true if is codeblock or closure
{
    Value *s = a->stack.peekptr();
    Types::tInteger->set(s,s->t->isCallable()?1:0);
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

%word i (-- current) get current iterator value (key if hash)
{
    Value *p = a->pushval();
    p->copy(a->getTopIterator()->current);
}
%word j (-- current) get nested iterator value (key if hash)
{
    Value *p = a->pushval();
    p->copy(a->getTopIterator(1)->current);
}
%word k (-- current) get nested iterator value (key if hash)
{
    Value *p = a->pushval();
    p->copy(a->getTopIterator(2)->current);
}

%word ival (-- current) get current iterator value (value if hash)
{
    Value *iterable = a->getTopIterator()->iterable;
    Value *cur = a->getTopIterator()->current;
    Value *p = a->pushval();
    iterable->t->getValue(iterable,cur,p);
}

%word jval (-- current) get nested iterator value (value if hash)
{
    Value *iterable = a->getTopIterator()->iterable;
    Value *cur = a->getTopIterator()->current;
    Value *p = a->pushval();
    iterable->t->getValue(iterable,cur,p);
}

%word kval (-- current) get nested iterator value (value if hash)
{
    Value *iterable = a->getTopIterator()->iterable;
    Value *cur = a->getTopIterator()->current;
    Value *p = a->pushval();
    iterable->t->getValue(iterable,cur,p);
}

%word iter (-- iterable) get the iterable which is currently being looped over
{
    Value *p = a->pushval();
    IteratorObject *iterator = a->getTopIterator();
    p->copy(iterator->iterable);
}

%word mkiter (iterable -- iterator) make an iterator object
{
    Value *p = a->stack.peekptr();
    p->t->createIterator(p,p);
}

%word icur (iterator -- item) get current item in iterator made with mkiter
{
    Value *p = a->stack.peekptr();
    Iterator<Value *>* i = Types::tIter->get(p);
    if(i->isDone())
        p->clr();
    else
        p->copy(i->current());
}

%word inext (iterator --) advance the iterator
{
    Value *p = a->stack.popptr();
    Iterator<Value *>* i = Types::tIter->get(p);
    i->next();
}

%word icurnext (iterator -- item) combined icur and inext
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
{
    Value *p = a->stack.popptr();
    Iterator<Value *>* i = Types::tIter->get(p);
    i->first();
}

%word idone (iterator -- boolean) true if iterator is done
{
    Value *p = a->stack.peekptr();
    Iterator<Value *>* i = Types::tIter->get(p);
    Types::tInteger->set(p,i->isDone()?1:0);
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
    const StringBuffer &name = a->popString();
    const char *s = a->getSpec(name.get());
    if(!s)s="no help found";
    printf("%s: %s\n",name.get(),s);
}

%word listhelp (s --) list and show help for all public functions in the given namespace
{
    Namespace *s = a->names.getSpaceByName(a->popString().get());
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
{
    Type::dumpTypes();
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

%word gc (--) perform a major garbage detect and cycle removal
{
    a->gc();
}

%word nspace (name -- handle) get a namespace by name
{
    const StringBuffer& name = a->popString();
    Namespace *ns = a->names.getSpaceByName(name.get());
    a->pushInt(ns->idx);
}

%word names (handle --) get a list of names from a namespace
{
    int idx = a->popInt();
    Namespace *ns = a->names.getSpaceByIdx(idx);
    
    ArrayList<Value> *list=Types::tList->set(a->pushval());
    ns->appendNamesToList(list);
    
}


%word ispriv (handle name -- bool) return true if the definition is private in the namespace
{
    NamespaceEnt *ent = getNSEnt(a);
    a->pushInt(ent->isPriv?1:0);
    
}
%word isconst (handle name -- bool) return true if the definition is constant in the namespace
{
    NamespaceEnt *ent = getNSEnt(a);
    a->pushInt(ent->isConst?1:0);
    
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


%word endpackage () mark end of package, only when used within a single script
{
    a->endPackageInScript();
}


%word dumpframe () debugging - dump the frame variables
{
    a->dumpFrame();
    CycleDetector::getInstance()->dump();
}

%word bor (a b -- a|b) bitwise or
{
    Value *p[2];
    a->popParams(p,"nn");
    int x = p[0]->toInt();
    int y = p[1]->toInt();
    a->pushInt(x|y);
}
%word band (a b -- a&b) bitwise and
{
    Value *p[2];
    a->popParams(p,"nn");
    int x = p[0]->toInt();
    int y = p[1]->toInt();
    a->pushInt(x&y);
}
%word bxor (a b -- a^b) bitwise xor
{
    Value *p[2];
    a->popParams(p,"nn");
    int x = p[0]->toInt();
    int y = p[1]->toInt();
    a->pushInt(x^y);
}
%word bnot (a -- ~a) bitwise not
{
    Value *p;
    a->popParams(&p,"n");
    int x = p->toInt();
    a->pushInt(~x);
}
%word shiftleft (a b -- a<<b) bitshift left
{
    Value *p[2];
    a->popParams(p,"nn");
    int x = p[0]->toInt();
    int y = p[1]->toInt();
    a->pushInt(x<<y);
}
%word shiftright (a b -- a>>b) bitshift right
{
    Value *p[2];
    a->popParams(p,"nn");
    int x = p[0]->toInt();
    int y = p[1]->toInt();
    a->pushInt(x<<y);
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
    a->registerProperty("autogc",new angort::AutoGCProperty(a));
    a->registerProperty("searchpath",new angort::SearchPathProperty(a));
}
    
