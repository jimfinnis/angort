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

typedef void (*NativeFunc)(class Angort *a);

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

extern TokenRegistry tokens[];

struct CodeBlock;

/// size of the primary stack
#define MAINSTACKSIZE  128
/// the default automatic GC interval, which can be changed by
/// autogc property (or stopped with a value of -1)
#define AUTOGCINTERVAL 100000
/// the default search path for plugins
#define DEFAULTSEARCHPATH ".:~/.angort:/usr/local/share/angort"

/// make the name of a library structure as created by makeWords.pl
#define LIBNAME(x) _angortlib_ ## x



/// this is a structure of word names / pointers as produced
/// by makeWords.pl. 

struct WordDef {
    const char *name; //!< word name (or null to terminate list)
    const char *desc; //!< description text
    NativeFunc f; //!< function pointer
};

/// a structure describing a library as produced by makeWords.pl

struct LibraryDef {
    const char *name; //!< library name (i.e. the angort Namespace)
    WordDef *wordList; //!< list of words, terminated with a null name
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
#endif
    union {
        int i;
        float f;
        const char *s;
        NativeFunc func;
        const CodeBlock *cb;
        Property *prop;
        struct {
            int l; //!< how many locals in total
            int p; //!< how many of those are params to pop
        } locals;
    }d;
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
    
    CompileContext(){
        closureListCt=0;
        spec=NULL;
        closureList=NULL;closureListTail=NULL;
        cb=NULL;
        reset(NULL,NULL);
    }
    
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
             
    
    /// make a permanent copy of the instruction buffer
    Instruction *copyInstructions(){
        Instruction *buf = new Instruction[compileCt];
        memcpy(buf,compileBuf,compileCt*sizeof(Instruction));
        return buf;
    }
    
    /// add a new local, initially just a stack variable.
    int addLocalToken(const char *s){
        int t = localTokenCt;
        if(localTokenCt==MAXLOCALS)
            throw RUNT("too many local tokens");
        localIndices[localTokenCt]=localTokenCt;
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
    
    void pushleave(){
        leaveListStack.push(leaveListHead); // push the current leave list onto the stack.
        leaveListHead = -1; // and start a new one
    }
    
    void popleave(){
        leaveListHead = leaveListStack.pop();
    }
          
    void checkStacksAtEnd(){
        if(!cstack.isempty())
            throw SyntaxException("'if' or iterator left unclosed?");
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
    
    void setFromContext(CompileContext *con){
        ip = con->copyInstructions();
        locals = con->getLocalCount();
        params = con->getParamCount();
        size = con->getCodeSize();
        closureTable = con->makeClosureTable(&closureTableSize);
        closureBlockSize = con->closureCt;
        localsClosed = con->localsClosed;
        used=true;
    }
    
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
    
    /// indicates which locals are closed, and therefore
    /// indexed into the closuretable
    uint32_t localsClosed;
    
    bool used; //!< true if the codeblock ends up being used (so the context mustn't delete it)
};


#define VSTACKSIZE 256

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
    Value vars[VSTACKSIZE];
    Stack<Tuple,32> baseStack;
    int base,next;
public:
    VarStack(){
        base=0;
        next=0;
    }
    
    void clear(){
        base=0;
        next=0;
        for(int i=0;i<VSTACKSIZE;i++){
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
        //        printf("state now %d/%d\n",base,next);
    }
    
    void pop(){
        Tuple *p=baseStack.popptr();
        base = p->base;
        next = p->next;
        //        printf("popping state: state now %d/%d\n",base,next);
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
    Value rec; //!< the recursion data (i.e. "this function")
    Value clos; //!< stores any closure created at this level
    
    void clear(){
        rec.clr();
        clos.clr();
    }
};



/// This is the main Angort class, of which there should be only
/// one instance.

class Angort {
    friend struct CodeBlock;
    friend class AutoGCProperty;
    friend class SearchPathProperty;
    static Angort *callingInstance; ///!< set when feed() is called.
private:
    bool running; //!< used by shutdown()
    Stack<Frame,32> rstack; //!< the return stack
    Value currClosure; //!< the closure block of the current level
    
    Stack<Value,8> loopIterStack; // stack of loop iterators
    
    Stack<CompileContext,4> contextStack;
    VarStack locals;
    
    ArrayList<LibraryDef *> *libs; //!< list of libraries
    
    int stdNamespace; //!< the default "std" namespace index
    const char *searchPath; //!< colon-separated library search path
    
    int autoCycleCount; //!< current auto GC count
    
    /// if an exception occurred during run, this will have 
    /// the last instruction.
    const Instruction *ipException;
    
   /// have we already done "package" in this file?
    bool definingPackage;
    /// the current compile context
    CompileContext *context;
    
    Tokeniser tok;
    const Instruction *ip,*debugwordbase;
    
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
    
    /// add a new instruction to the current compile context
    Instruction *compile(int opcode){
        return context->compile(opcode);
    }
    
    /// push the current compile context onto the stack and clear the new one
    void pushCompileContext(){
        CompileContext *p = context;
        context = contextStack.pushptr();
        context->reset(p,&tok);
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
    
    /// show an instruction
    void showop(const Instruction *ip,const Instruction *base);
    
    /// this will push the locals stack
    /// and push the rstack. The new IP
    /// is returned, and the old one is passed in
    /// (which could be NULL for top level).
    const Instruction *call(const Value *v, const Instruction *returnip);
    
    /// find a global or create one if it doesn't exist;
    /// used for autoglobals.
    int findOrCreateGlobal(const char *name){
        int i = names.get(name);
        if(i<0)
            i = names.add(name);
        return i;
    }
    
    /// add a plugin (Linux only, uses shared libraries). Returns
    /// the new namespace ID.
    int plugin(const char *path);
    
    
    /// called at the end of a block of code,
    /// or by emergency stop invocation. Returns
    /// the new IP or NULL.
    const Instruction *ret();
    
    /// clear all stacks etc.
    void clearAtEndOfFeed();
    
    /// look for a file in the search path. Will attempt to use wordexp
    /// to do shell expansions of the path if it is available.
    const char *findFile(const char *name);
    
public:
    /// if non-neg, GC cycle detect is called after this number of instructions
    int autoCycleInterval; 
    
    /// this returns the top level of angort which was called;
    /// it's still possible to have multiple angorts running,
    /// but this is set when feed() is called. It's really ugly.
    /// but I can see no other way to handle things like HashType::toString()
    /// except by putting the angort pointer as a parameter everywhere,
    /// or using a singleton.
    static Angort *getCallingInstance(){
        return callingInstance;
    }
    
    /// used to run a codeblock - works by doing call() and then run() until exit.
    /// Will not push return stack.
    void runValue(const class Value *v);
    
    /// if an exception occurred in a run, this will have the IP.
    const Instruction *getIPException(){
        return ipException;
    }
    
    /// show a stack trace
    void trace();
    
    /// handle binary operations (public; used in comparators)
    void binop(Value *a,Value *b,int opcode);
    
    
    /// call this to get the version number. It's a denary integer,
    /// the lowest two digits of which are the minor version. It's
    /// a number because it's used in files.
    static int getVersion();
    
    Stack<Value,128>stack;
    bool emergencyStop;
    /// if true, unidentified idents will be converted to strings
    bool barewords;
    /// debug flags
    /// 1 - show instructions as they run
    /// 2 - show parsing in the tokeniser
    int debug;
    /// make assertions print statements even when they pass just fine,
    /// used in testing.
    bool assertDebug;
    /// if true, assertion conditions are negated - useful for testing
    /// that something should assert
    bool assertNegated;
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
        
    /// dump the stack to stdout
    void dumpStack(const char *s);
    
    /// dump the frame data
//    void dumpFrame();
    
    
    /// get the top iterator on the iterator stack (or the nth)
    IteratorObject *getTopIterator(int i=0){
        Value *v = loopIterStack.peekptr(i);
        if(v->t != Types::tIter)
            throw RUNT("attempt to get i,j,k or iter when not in an iterable loop");
        return v->v.iter;
    }
    
    /// clear the entire system
    void clear(){
        names.clear();
        stack.clear();
        locals.clear();
    }          
    
    /// clear the stack
    void clearStack(){
        stack.clear();
    }
        
    
    /// are we in a non-root compile context (i.e. compiling a bracketed
    /// word?)
    bool inSubContext(){
        return contextStack.ct > 1;
    }
    const char *getLastLine(){
        return lastLine;
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
    void pushFloat(float f){
        Types::tFloat->set(stack.pushptr(),f);
    }
    void pushString(const char *s){
        Types::tString->set(stack.pushptr(),s);
    }
    
    void pushNone(){
        stack.pushptr()->clr();
    }
    
    int popInt(){
        return Types::tInteger->get(popval());
    }
    float popFloat(){
        return Types::tFloat->get(popval());
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
    
    /// function for registering properties
    void registerProperty(const char *name, Property *p, const char *ns=NULL,const char *spec=NULL);
    
    /// register a whole bunch of words. Returns the new namespace index,
    /// which can then be imported; or the whole library can be 
    /// imported by setting the bool to true.
    
    int registerLibrary(LibraryDef *lib,bool import=false);
    
    
    Angort();
    
    /// the destructor just calls shutdown if Angort isn't shutdown
    /// already - this is so we can get away with just calling exit()
    /// after shutdown() in "quit".
    ~Angort(); 
    /// actually does destruction - see the destructor.
    void shutdown();
    
    /// just compile a string into a set of instructions
    const Instruction *compile(const char *s);
    
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
    
    
    /// run until OP_END received and no return stack
    void run(const Instruction *ip);
    
    /// disassemble a named word
    void disasm(const char *name);
    
    /// list all words, globals and consts
    void list();
    
    /// get the spec string for a word or native
    const char *getSpec(const char *s);
    
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
    
    /// run the cycle detector
    void gc(){
        GarbageCollected::gc();
    }
        
    
};


}
#endif /* __ANGORT_H */
