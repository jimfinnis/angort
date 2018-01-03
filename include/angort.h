/**
 * \file 
 *
 * 
 * \author $Author$
 * \date $Date$
 */


#ifndef __ANGORT_H
#define __ANGORT_H

namespace angort {

typedef void (*NativeFunc)(class Runtime *a);

}

#include <stdint.h>
#include "stack.h"
#include "stringbuf.h"
#include "tokeniser.h"
#include "map.h"
#include "types.h"
#include "value.h"
#include "namespace.h"

namespace angort {

/// true to compile debugging data into opcodes
#define SOURCEDATA 1

/// total number of local variables
#define VBLOCKSIZE 4096
/// depth of return stack
#define RSTACKSIZE 256

extern TokenRegistry tokens[];

struct CodeBlock;

/// size of the primary stack
#define MAINSTACKSIZE  128
/// the default automatic GC interval, which can be changed by
/// autogc property (or stopped with a value of -1)
#define AUTOGCINTERVAL 100000
/// the default search path for plugins
#define DEFAULTSEARCHPATH ".:~/.angort:/usr/local/share/angort:~/share/angort"

/// make the name of a library structure as created by makeWords.pl
#define LIBNAME(x) _angortlib_ ## x


/// this is a structure of word names / pointers as produced
/// by makeWords.pl. 

struct WordDef {
    const char *name; //!< word name (or null to terminate list)
    const char *desc; //!< description text
    NativeFunc f; //!< function pointer
};

/// a structure for binary operators as produced by makeWords.pl

struct BinopDef {
    const char *lhs; //!< name of LHS type
    const char *rhs; //!< name of LHS type
    const char *opcode; //!< opcode of operator
    BinopFunction f; //!< function pointer
};

/// a structure describing a library as produced by makeWords.pl

struct LibraryDef {
    const char *name; //!< library name (i.e. the angort Namespace)
    WordDef *wordList; //!< list of words, terminated with a null name
    BinopDef *binopList; //!< list of binops, terminated with null lhs
    NativeFunc initfunc; //!< possibly null initialisation function
    NativeFunc shutdownfunc; //!< possibly null shutdown function
};
    


/// a property - it's a value which can be accessed like
/// a global variable, but has sideeffects on get or set.
/// User should override.
struct Property {
    Value v;
    virtual void postSet(){}; // v will have been set to value
    virtual void preGet(){}; // should set v if required
    
    virtual void preSet(){}; // so we can pick up extra params
    virtual void postGet(){}; // after the get has been done
};


/// all code is a sequence of these - an opcode and some data
struct Instruction {
    
    /// for debugging
    const char *getDetails(char *buf,int len) const;
    
    int opcode;
#if SOURCEDATA
    const char *file;
    int line,pos;
    bool brk;
#endif
    union {
        int i;
        float f;
        double df;
        long l;
        const char *s;
        NativeFunc func;
        const CodeBlock *cb;
        Property *prop;
        class Value *constexprval;
        IntKeyedHash<int> *catches; // for OP_TRY
    }d;
};

/// structure used in compiling exceptions, generated
/// for each "catch" clause. Consists of the symbols caught,
/// and the start and end offsets of the catch clause.

struct Catch {
    /// list of symbols which this catch pair catches
    ArrayList<int> symbols;
    int start,end; //!< start and end of exception
    
    void clear(){
        symbols.clear();
    }
};




/// MUST be <32, since we use a 32-bit int as a set of booleans
/// in various places
#define MAXLOCALS	32

/// this structure is attached to contexts which refer to closures,
/// including the codeblocks which defines the variable itself.
/// It contains data about which cb the variable comes from,
/// and which variable within the closure block it is.
struct ClosureListEnt {
    const CodeBlock *c; // parent cb defining variable
    int i; // index in closure block
    ClosureListEnt *next; // link
    
    ClosureListEnt(CodeBlock *_c,int _i){
        c = _c; i = _i;
    }
};

/// an array of these is built for any codeblocks which
/// require access to closure variables
struct ClosureTableEnt {
    ClosureTableEnt(){
        levelsUp=-1;
        idx=-1;
    }
    int levelsUp; //!< how many levels up the call stack the closure block is
    int idx; //!< the index into the appropriate closure block
};

/// a compilation context - we have a stack of these so that we can
/// do lambdas

class CompileContext {
    int compileCt; //!< words so far in compile buffer
    Instruction compileBuf[1024]; //!< compile buffer
    Stack<int,8> cstack; //!< location stack for loops etc.
    Stack<int,8> leaveListStack; //!< stack of leave instruction lists - linked through the d.i field, this is a list of OP_LEAVE etc. which must be resolved when a loop ends.
    
    char localTokens[MAXLOCALS][64]; //!< locals in the word currently being defined
    int localTokenCt; //!< number of locals in the word currently being defined
    int paramCt; //!< the first N locals are parameters
    /// this is a map from local index (i.e. indices into localTokens) to
    /// opcode index - i.e. the index of the closure or local variable.
    char localIndices[MAXLOCALS];
    Type *localTypes[MAXLOCALS]; //!< types of parameters (could be NULL)
    
    /// the closure list for the current function, from which
    /// the closure table is made
    ClosureListEnt *closureList,*closureListTail;
    int closureListCt;
    
    CompileContext *parent; //!< pointer to containing context or NULL, set up by pushCompileContext
    int leaveListHead; //!< the head of a leave list - the index of the first OP_LEAVE etc. instruction, or -1.
    
    Tokeniser *tokeniser; //!< the tokeniser
    
    /// convert a variable/parameter into a closure - we need
    /// to scan through existing variables too!
    void convertToClosure(const char *name);
    
    int addClosureListEnt(CodeBlock *c,int n){
        cdprintf("adding new closure list entry, codeblock %p number %d",c,n);
        ClosureListEnt *p = new ClosureListEnt(c,n);
        p->next = NULL;
        
        if(!closureList){
            closureList = p;
            closureListTail = p;
        } else {
            closureListTail->next = p;
            closureListTail = p;
        }
        
        return closureListCt++;
    }
    
    // this is used when we try to get the index of an variable in
    // a containing context's closure block from its index in that context's
    // closure table.
    ClosureListEnt *getClosureListEntByIdx(int n){
        cdprintf("Scanning for the block location of table (list) entry %d",n);
        ClosureListEnt *p = closureList;
        for(int i=0;i<n;i++){
            // this is thrown when we don't have enough entries in the table -
            // like trying to get index 1 in a length 1 table.
            if(!p)throw WTF;
            p=p->next;
        }
        return p;
    }
    
    void dumpClosureList(){
        int i=0;
        ClosureListEnt *p;
        for(p=closureList;p;p=p->next,i++){
            cdprintf("Closure list entry %d: codeblock %p, index %d",i,p->c,p->i);
        }
        
    }
    
    void freeClosureList(){
        ClosureListEnt *p,*q;
        if(closureList){
            for(p=closureList;p;p=q){
                q=p->next;
                delete p;
            }
            closureList=NULL;closureListTail=NULL;
        }
        closureListCt=0;
    }
          
    
public:
    
    const char *spec; //!< specification string, which we strdup.
    CodeBlock *cb; //!< the codeblock this context is compiling
    /// a bitfield indicating which of the variables should be closed;
    /// they are referred to in subfunctions are therefore should be stored
    /// in a closure.
    uint32_t localsClosed;
    /// this should be the number of bits set in localsClosed
    int closureCt;
    
    Stack<ArrayList<Catch>,4> catchSetStack; //!< stack for exception compiling
    Catch *curcatch; //!< the catch we're currently working with
    
    class Angort *ang; //!< must be set by pushCompileContext()
    CompileContext(){
        cstack.setName("compile");
        leaveListStack.setName("leavelist");
    
        closureListCt=0;
        spec=NULL;
        closureList=NULL;closureListTail=NULL;
        cb=NULL;
        reset(NULL,NULL);
    }
    
    void cdprintf(const char *s,...){
#if 0
        char buf[256];
        va_list args;
        va_start(args,s);
        vsprintf(buf,s,args);
        printf("[context %p with cb %p]  %s\n",this,cb,buf);
#endif
    }
        
    
    void dump();
    
    /// reset a new compile context and set the containing context.
    void reset(CompileContext *p,Tokeniser *tok);
    
    /// build a closure table which can be set into the codeblock,
    /// out of the closure list (which can then be deleted).
    /// The size of the table is set in count.
    ClosureTableEnt *makeClosureTable(int *count);
    
    /// scan functions above this for a variable, and convert to a closure if
    /// required. Having found the closure, add an entry to this context's closure table
    /// (creating one if required) if it's not already there.
    int findOrCreateClosure(const char *name);
    
    /// add the current instruction to the leave list
    void compileAndAddToLeaveList(int opcode){
        Instruction *i = compile(opcode);
        i->d.i = leaveListHead;
        leaveListHead = compileCt-1; // previous instruction, we've incremented
    }
    
    void dumpLeaveList(const char *s){
        printf("Leave list %s:\n",s);
        for(Instruction *i=leaveListFirst();i;i=leaveListNext(i))
            printf("    %p [opcode %d]\n",i,i->opcode);
    }
    
    Instruction *leaveListFirst(){
        if(leaveListHead<=0)
            return NULL;
        return compileBuf+leaveListHead;
    }
    Instruction *leaveListNext(Instruction *i){
        if(i->d.i<=0)
            return NULL;
        else
            return compileBuf+(i->d.i);
    }
    
    void clearLeaveList(){
        leaveListHead=-1;
    }
    
    void closeAllLocals();
    
    /// make a permanent copy of the instruction buffer
    Instruction *copyInstructions(){
        Instruction *buf = new Instruction[compileCt];
        memcpy(buf,compileBuf,compileCt*sizeof(Instruction));
        return buf;
    }
    
    /// add a new local, initially just a stack variable.
    /// Type checking is only for parameters currently.
    int addLocalToken(const char *s,Type *typ){
        int t = localTokenCt;
        if(localTokenCt==MAXLOCALS)
            throw RUNT(EX_TOOMANYLOCALS,"too many local tokens");
        cdprintf(" Local index for %s added as %d",s,localTokenCt);
        localIndices[localTokenCt]=localTokenCt;
        localTypes[localTokenCt]=typ;
        strcpy(localTokens[localTokenCt++],s);
        return t;
    }
    
    int getLocalCount(){
        return localTokenCt;
    }
    
    /// gets set by compiling parameters in compileParamsAndLocals()
    void setParamCount(int p){
        paramCt=p;
    }
    
    int getParamCount(){
        return paramCt;
    }
    
    
    
    /// return the index of a local token, or -1 if it's not in the table
    /// NOTE THAT you need to use the appropriate closure or local index,
    /// by looking at isClosed() and getLocalIndex()
    int getLocalToken(const char *s){
        for(int i=0;i<localTokenCt;i++){
            if(!strcmp(s,localTokens[i]))
                return i;
        }
        return -1;
    }
    
    /// given local token index, return whether closed
    bool isClosed(int t){
        return 0 != ((1<<t) & localsClosed);
    }
    
    /// given local token index, return index of either local or closure index.
    int getLocalIndex(int t){
        return localIndices[t];
    }
    
    /// get the type of a local (normally a parameter)
    Type *getLocalType(int t){
        return localTypes[t];
    }
            
    
    
    Instruction *compile(int opcode){
        if(compileCt==1024)
            throw SyntaxException("word too long");
        Instruction *i = compileBuf+compileCt;
        compileCt++;
        i->opcode = opcode;
#if SOURCEDATA
        i->file = tokeniser->getname();
        i->line = tokeniser->getline();
        i->pos = tokeniser->getpos();
        i->brk = false;
#endif
        return i;
    }
    
    Instruction *getCode(){
        return compileBuf;
    }
    
    int getCodeSize(){
        return compileCt;
    }
    
    /// push the current location onto the compile stack
    void pushhere(){
        cstack.push(compileCt);
    }
    
    /// push a marker
    void pushmarker(){
        cstack.push(-1);
    }
    
    void pushleave(){
        leaveListStack.push(leaveListHead); // push the current leave list onto the stack.
        leaveListHead = -1; // and start a new one
    }
    
    void popleave(){
        leaveListHead = leaveListStack.pop();
    }
          
    void checkStacksAtEnd(){
        if(!cstack.isempty())
            throw SyntaxException("structure left unclosed?");
    }
        
    
    /// pop the location off the compile stack
    int pop(){
        return cstack.pop();
    }
    
    /// set the offset of the jump instruction at idx to be a jump
    /// to the instruction N instructions (default 0) past the current one
    void resolveJumpForwards(int idx, int offset=0){
        compileBuf[idx].d.i = (compileCt-idx)+offset;
    }
    
    Instruction *getInst(int idx){
        return compileBuf+idx;
    }
    
    /// copy a specification string in
    void setSpec(const char *s){
        spec = (const char *)strdup(s);
    }
    
    /// get the spec, which (if not null) we will have allocated
    const char *getSpec(){
        return spec;
    }
    
};


/// a code block consists of a pointer to some instructions, a count of local variables,
/// a count of how many of those should be popped off the stack (i.e. are parameters).

struct CodeBlock {
    
    CodeBlock(){
        used=false;
    }
    
    void setFromContext(CompileContext *con);
    
    /// the number of instructions in the codeblock
    int size;
    /// the instructions themselves, must be delete[]ed    
    const Instruction *ip; 
    
    /// describes the location of closed variables, by index in closure block and
    /// how far up the call stack the closure block is. This is used to generate
    /// a closure map on instantiation of the codeblock.
    const ClosureTableEnt *closureTable;
    /// the size of the closure table
    int closureTableSize;
    /// the number of closures local to this variable
    int closureBlockSize;
    
    /// this is the total number of local variables,
    /// including closures
    int locals;
    /// the first N of the locals/closures should be popped from the
    /// stack when this CB is called
    int params;
    
    /// the indices of the parameters in the local or closure map.
    uint8_t *paramIndices;
    
    /// indicates which locals are closed, and therefore
    /// indexed into the closuretable
    uint32_t localsClosed;
    
    /// parameter types (which each may be null for no checking)
    Type **paramTypes; 
    
    bool used; //!< true if the codeblock ends up being used (so the context mustn't delete it)
};


/// this class handles the stack of local variables used by
/// the system. It has a base pointer, pointing at the first
/// variable allocated by the currently running code;
/// and a next pointer, where variables for the next word
/// will be allocated from. Each time a we go down a level,
/// we push the current base/next pair onto an internal stack,
/// and then make base=next, indicating that we are now
/// in the new frame and no locals have been allocated. OP_LOCALS
/// may then be called, causing next to be incremented by allocating
/// locals. When we leave a frame, the previous state is popped from
/// the internal stack.
class VarStack {
    
    struct Tuple{
        int base,next;
    };
    
private:
    Value vars[VBLOCKSIZE];
    Stack<Tuple,RSTACKSIZE> baseStack;
    int base,next;
public:
    VarStack(){
        baseStack.setName("variable");
        base=0;
        next=0;
    }
    
    void clear(){
        base=0;
        next=0;
        for(int i=0;i<VBLOCKSIZE;i++){
            vars[i].clr();
        }
        baseStack.clear();
    }
        
    
    void push(){
        //        printf("pushing stack: curstate = base %d/next %d ",
        //               base,next);
        Tuple *p = baseStack.pushptr();
        p->base = base;
        p->next = next;
        base=next;
//                printf("state now %d - %d/%d\n",baseStack.ct,base,next);
    }
    
    void pop(){
        Tuple *p=baseStack.popptr();
        base = p->base;
        next = p->next;
//                printf("popping state: state now %d - %d/%d\n",baseStack.ct,base,next);
    }
    
    /// allocate space after push()
    void alloc(int localct){
        next+=localct;
    }
    
    /// store value into a local slot
    void store(int n,Value *v){
        vars[base+n].copy(v);
    }
    
    
    
    Value *get(int n){
        return vars+base+n;
    }
};

/// this is a stack frame; we push this every time we go into
/// a function and pop every time we return.
struct Frame {
    const Instruction *ip; //!< the return address
    const Instruction *base; //!< the address of the start of the function
    Value rec; //!< the recursion data (i.e. "this function")
    Value clos; //!< stores any closure created at this level
    /// how many loop iterators are stacked for loops in this
    /// frame. This number is popped off if the function runs OP_STOP.
    int loopIterCt; 
    
    
    void clear(){
        rec.clr();
        clos.clr();
        loopIterCt=0;
    }
};


/// thread hook class - the thread library installs one of these
/// into Angort using setThreadHookObject()

class ThreadHookObject {
public:
    // single global mutex, nestable
    virtual void globalLock() = 0;
    virtual void globalUnlock() = 0;
};

/// runtime data

class Runtime {
    friend class Angort;
public:
    
    Runtime(Angort *angort,const char *name="anon");
    virtual ~Runtime();
    
    const Instruction *ip,*wordbase;
    Stack<Value,128>stack;
    bool emergencyStop;
    int id;
    const char *name;
private:
    Stack<Frame,RSTACKSIZE> rstack; //!< the return stack
    Value currClosure; //!< the closure block of the current level
    /// how many loop iterators are stacked for loops in this
    /// frame. This number is popped off if the function runs OP_STOP.
    int loopIterCt; 
    /// stack of stacks of exception handlers. The outer stack
    /// matches the rstack (annoyingly I can't put it in there without
    /// lots of copy operations), while the inner one is the stack
    /// for within the function. This is big and inefficient,
    /// as it contains loads of empty hashes!
    Stack<Stack<IntKeyedHash<int>*,4>,RSTACKSIZE+1> catchstack;
    
    Stack<Value,RSTACKSIZE> loopIterStack; // stack of loop iterators
    VarStack locals;
    /// this will push the locals stack
    /// and push the rstack. The new IP
    /// is returned, and the old one is passed in
    /// (which could be NULL for top level).
    const Instruction *call(const Value *v, const Instruction *returnip);
    
    
    /// called at the end of a block of code,
    /// or by emergency stop invocation. May set the IP to NULL.
    void ret();

public:
    class Angort *ang; // main angort object
    // annoyingly public to allow debugger access
    bool debuggerNextIP; // stop at the next IP?
    bool debuggerStepping; // don't clear debuggerNextIP after stop
    int autoCycleCount; //!< current auto GC count
    
     
    /// show an instruction (might seem weird that it's in Runtime, but it
    /// uses wordbase).
    void showop(const Instruction *ip,const Instruction *base=NULL,
                const Instruction *curr=NULL);
    /// used to run a codeblock - works by doing call() and then run() until exit.
    /// Will not push return stack.
    void runValue(const class Value *v);
    
    /// store a stack trace into an arraylist of allocated strings,
    /// which should be deleted. We do this to stash the trace before
    /// we start unwinding the stack as part of exception handling.
    void storeTrace();
    
    /// used with storeTrace(), this prints and deletes any stored trace.
    /// Only does the printing if traceOnException is set.
    void printAndDeleteStoredTrace();
    
    ArrayList<char *> storedTrace; //!< the stored trace
    
    /// used with storeTrace(), this prints stored trace.
    /// Only does the printing always.
    void printStoredTrace();
    
    /// prints an immediate trace; doesn't store.
    void printTrace();
    
    /// handle binary operations (public; used in comparators)
    void binop(Value *a,Value *b,int opcode);
    
    /// show each instruction as it runs
    bool trace;
    /// make assertions print statements even when they pass just fine,
    /// used in testing.
    bool assertDebug;
    /// if true, assertion conditions are negated - useful for testing
    /// that something should assert
    bool assertNegated;
    
    /// print trace on exception in run()
    bool traceOnException;
    
    /// stream used for output in redirection
    FILE *outputStream;
    void endredir(){
        if(outputStream != stdout){
            fclose(outputStream);
            outputStream=stdout;
        }
    }
    
    /// dump the stack to stdout
    void dumpStack(const char *s);
    
    /// dump the frame data
    void dumpFrame();
    
    
    /// get the top iterator on the iterator stack (or the nth)
    IteratorObject *getTopIterator(int i=0){
        Value *v = loopIterStack.peekptr(i);
        if(v->t != Types::tIter)
            throw RUNT(EX_NOITERLOOP,"attempt to get i,j,k or iter when not in an iterable loop");
        return v->v.iter;
    }
    
    /// clear the runtime
    void clear(){
        stack.clear();
        locals.clear();
    }          
        
    /// clear the stack
    void clearStack(){
        stack.clear();
    }
    
    
    /// pop an item and return a pointer to it
    Value *popval(){
        return stack.popptr();
    }
    
    /// push the stack, and return a pointer to the value
    /// to be filled in
    Value *pushval(){
        return stack.pushptr();
    }
    
    // common type push/pop shorthand
    
    void pushInt(int i){
        Types::tInteger->set(stack.pushptr(),i);
    }
    void pushLong(long i){
        Types::tLong->set(stack.pushptr(),i);
    }
    void pushFloat(float f){
        Types::tFloat->set(stack.pushptr(),f);
    }
    void pushDouble(double f){
        Types::tDouble->set(stack.pushptr(),f);
    }
    void pushString(const char *s){
        Types::tString->set(stack.pushptr(),s);
    }
    
    void pushNone(){
        stack.pushptr()->clr();
    }
    
    bool popBool(){
        return popval()->toBool();
    }
    
    int popInt(){
        return Types::tInteger->get(popval());
    }
    
    float popFloat(){
        return Types::tFloat->get(popval());
    }
    double popDouble(){
        return Types::tDouble->get(popval());
    }
    StringBuffer popString(){
        return StringBuffer(popval());
    }
    
    /// pop parameters for a C++ function, with type checking.
    /// Each parameter is checked against the next character in the type
    /// string, and an exception is thrown if there is a mismatch
    /// which specifies which parameter failed.
    /// The type characters are:
    /// - n : numeric
    /// - c : callable
    /// - s : string or symbol
    /// - S : symbol only
    /// - v : variable (i.e. no checking)
    /// - l : list
    /// - h : hash
    /// - a : type passed in as type0
    /// - b : type passed in as type1
    /// - A : type passed in as type0 or none
    /// - B : type passed in as type1 or none
    /// Each new parameter is added into the Value pointer array passed
    /// in - these are direct pointers into the Angort main stack.
    /// MAKE SURE THE OUT ARRAY CONTAINS AT ENOUGH ROOM.
    void popParams(Value **out,const char *spec,const Type *type0=NULL,
                   const Type *type1=NULL);
    
    /// run until OP_END received and no return stack
    void run(const Instruction *startip);
    
    /// stop any running code (call from a signal handler, or
    /// code inside a word to terminate loops etc.)
    void stop(){
        emergencyStop=true;
    }
    
    /// you'll have to reset the emergency stop when you get
    /// control back (feed() will do it for you)
    void resetStop(){
        emergencyStop=false;
    }
    
    /// throw an exception, setting the IP of the handler if one
    /// is found, otherwise returning leaving it alone. Returns
    /// true if a handler was set, in which case the caller resets
    /// IP to NULL, possibly invoking the debugger first.
    bool throwAngortException(int symbol, Value *data);
    
    /// run the cycle detector
    void gc();
    
    /// ensure we are in thread zero (the default thread)
    void checkzerothread(){
        if(id)throw RUNT("ex$badthread","called from nonzero thread");
    }
    
    /// clear return stack and data at end of feed
    void clearAtEOF();
};




/// This is the main Angort class, of which there should be only
/// one instance.

class Angort {
    friend struct CodeBlock;
    friend class Runtime;
    friend class AutoGCProperty;
    friend class SearchPathProperty;
private:
    
    // if a thread API is installed, these run the various
    // thread handling methods using it.
    inline void globalLock(){
        if(threadHookObj)threadHookObj->globalLock();
    }
    inline void globalUnlock(){
        if(threadHookObj)threadHookObj->globalUnlock();
    }
    
    bool running; //!< used by shutdown()
    ThreadHookObject *threadHookObj;
    
    Stack<CompileContext,8> contextStack;
    ArrayList<LibraryDef *> *libs; //!< list of libraries
    
    int stdNamespace; //!< the default "std" namespace index
    const char *searchPath; //!< colon-separated library search path
    
    /// the current compile context
    CompileContext *context;
    
    Tokeniser tok;
    
    /// the index of the word currently being defined
    /// within the current namespace, whose value is set
    /// at the end of the definition.
    int wordValIdx;
    
    /// this defines a word with no instructions, and sets
    /// wordVal to point to the word's value. It's used at 
    /// the start of a word definition,
    void startDefine(const char *name);
    
    
    /// define a word from a context - startDefine() must have been called
    void endDefine(class CompileContext *cb);
    
    char lastLine[1024]; //!< last line read
    
    /// parse various tokens which are followed by a var name
    /// and have a lot of common code generation code.
    void parseVarAccess(int token);
    /// check that the given name (assumed to be the current token,
    /// tok.getstring(), for error printing), is not constant
    void constCheck(int name);
    
    /// add a new instruction to the current compile context
    Instruction *compile(int opcode){
        return context->compile(opcode);
    }
    
    /// push the current compile context onto the stack and clear the new one
    void pushCompileContext(){
        CompileContext *p = context;
        context = contextStack.pushptr();
        context->reset(p,&tok);
        context->ang = this;
    }
    
    /// pop a context off the stack, ready to carry on where we left off.
    /// We also return a pointer to the previous context, so we can reference
    /// its code. We do not reset it.
    CompileContext *popCompileContext(){
        context->checkStacksAtEnd();
        CompileContext *p = context;
        contextStack.popptr();
        context = contextStack.peekptr();
        return p;
    }
    
    
    /// compile a [params,locals] block, producing an OP_LOCALS instruction
    /// and filling the localTokens table with names to be used in this definition.
    /// Only works in defining mode!
    void compileParamsAndLocals();
    
    
    /// clear all stacks etc.
    void clearAtEndOfFeed();
    
    /// look for a file in the search path. Will attempt to use wordexp
    /// to do shell expansions of the path if it is available.
    const char *findFile(const char *name);
    
    /// autocomplete state
    ArrayList<const char *> *acList;
    int acIndex;
    
    /// heredoc end string or null
    char *hereDocEndString;
    /// heredoc string being build
    char *hereDocString;
    
public:
    Runtime *run; //!< the default runtime used by the main thread
    /// debugger hook, invoked by the "brk" word
    NativeFunc debuggerHook;
    
    /// set the object we use to manipulate threads
    void setThreadHookObject(ThreadHookObject *tho){
        threadHookObj = tho;
    }
    
    /// replace the debugger hook
    void setDebuggerHook(NativeFunc f){
        debuggerHook = f;
    }
    
    /// find a global or create one if it doesn't exist;
    /// used for autoglobals.
    int findOrCreateGlobal(const char *name){
        int i = names.get(name);
        if(i<0)
            i = names.add(name);
        return i;
    }
    
    Value *findOrCreateGlobalVal(const char *name){
        int g = findOrCreateGlobal(name);
        return names.getVal(g);
    }
          
    
    /// this is used by external programs which use angort as a 
    /// library to find/create a global and set it to a string
    /// value.
    void setGlobal(const char *name,const char *val){
        int g = findOrCreateGlobal(name);
//        printf("Global: %d\n",g);
        Types::tString->set(names.getVal(g),val);
    }
    
    /// add a plugin (Linux only, uses shared libraries). Returns
    /// the new namespace ID.
    int plugin(const char *path);
    
    /// if non-neg, GC cycle detect is called after this number of instructions
    int autoCycleInterval; 
    
    /// call this to get the version number.
    static const char *getVersion();
    
    /// if true, unidentified idents will be converted to strings
    bool barewords;
    bool tokeniserTrace;
    /// print each line we parse
    bool printLines;
    NamespaceManager names; //!< the namespaces are all handled by the namespace manager
    
    /// called at the end of a script which contains a package, where that
    /// package is not included by another script - effectively
    /// fakes the require return.
    void endPackageInScript();
    
    /// returns true if we are defining a word
    bool isDefining(){
        return wordValIdx>=0;
    }
    
    int lineNumber;
    
    int getLineNumber(){
        return lineNumber;
    }
        
    /// clear the entire system - will not clear runtime!
    void clear(){
        names.clear();
    }
    
    
    /// are we in a non-root compile context (i.e. compiling a bracketed
    /// word?)
    bool inSubContext(){
        return contextStack.ct > 1;
    }
    const char *getLastLine(){
        return lastLine;
    }
    
    /// function for registering properties
    void registerProperty(const char *name, Property *p, const char *ns=NULL,const char *spec=NULL);
    
    /// register a whole bunch of words. Returns the new namespace index,
    /// which can then be imported; or the whole library can be 
    /// imported by setting the bool to true.
    
    int registerLibrary(LibraryDef *lib,bool import=false);
    
    /// register a binary operation, given the typenames and opcode name.
    /// This will also accept "number" for a float/integer, and "str"
    /// for an integer/symbol.
    void registerBinop(const char *lhsName,const char *rhsName,
                       const char *opcode,BinopFunction f);
    
    Angort();
    
    /// the destructor just calls shutdown if Angort isn't shutdown
    /// already - this is so we can get away with just calling exit()
    /// after shutdown() in "quit".
    ~Angort(); 
    /// actually does destruction - see the destructor.
    void shutdown();
    
    /// just compile a string into a set of instructions
    void compile(const char *s,Value *out);
    
    /// feed a string into the interpreter
    void feed(const char *s);
    /// feed a whole file; will print a message and return
    /// false if there is a problem, or (and this is the default)
    /// throw an exception. Might be best to use include().
    bool fileFeed(const char *name,bool rethrow=true);
    
    /// file inclusion mechanism - if ispkg is set, the file
    /// should contain a package definition defining a new
    /// namespace, which will be left on the stack.
    void include(const char *path,bool ispkg);
    
    
    /// disassemble a named word
    void disasm(const char *name);
    
    /// disassemble a codeblock
    void disasm(const CodeBlock *cb);
    
    /// list all words, globals and consts
    void list();
    
    /// get the spec string for a word or native
    const char *getSpec(const char *s);
    
    /// reset the autocomplete list
    void resetAutoComplete();
    
    /// return the next possible autocomplete candidate
    const char *getNextAutoComplete();
    
    /// append a path to the search path, returning the prior
    /// path (which should be freed; the new path is malloced())
    const char *appendToSearchPath(const char *path);
    
    /// import all symbols in the `future namespace
    void importAllFuture();
    /// import all symbols in the `deprecated namespace
    void importAllDeprecated();
};


}
#endif /* __ANGORT_H */
