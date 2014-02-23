/**
 * \file 
 *
 * 
 * \author $Author$
 * \date $Date$
 */


#ifndef __ANGORT_H
#define __ANGORT_H

#include <stdint.h>
#include "stack.h"
#include "tokeniser.h"
#include "map.h"
#include "types.h"
#include "value.h"
#include "cvset.h"

/// the version number has the lowest two digits as minor version.
#define ANGORT_VERSION 207
/// first int in file for image data
#define ANGORT_MAGIC  0x737dfead

/// if this is defined, when an unknown identifier is encountered it
/// is stacked as a literal string.
#define BAREWORDS 1

extern TokenRegistry tokens[];

typedef void (*NativeFunc)(class Angort *a);

struct CodeBlock;


/// this is a closure - it's a CodeBlock (a function, if you will) associated with
/// the values which need to be bound to locals by the CodeBlock's closure map.
struct Closure : public GarbageCollected {
    Value codeBlockValue; //!< we store the codeblock as a value, to make serialisation painless.
    Value *table;
    int ct; // size of the table so we can copy it
    
    /// c may be NULL here, in which case the code block will
    /// not be initialised. Used in serialisation.
    Closure(const CodeBlock *c,int tabsize,Value *t);
    Closure(const Closure *c); // make a deep copy, allocating a new table and copy()ing all values.
    ~Closure();
};



/// this is a structure of word names / pointers as produced
/// by makeWords.pl. A null-terminated array of these can
/// be passed to registerWords().

struct AngortWordDef {
    const char *modname; //!< the name of the module in which this lives
    const char *name; //!< word name
    const char *desc; //!< description text
    NativeFunc f; //!< function pointer
};
/// and this is a macro to insert an external reference
/// to an array generated by makeWords.pl
#define WORDS(x) extern AngortWordDef _wordlist_##x[];
/// and this one will call registerWords on the angort
/// passed in, with the list given
#define REGWORDS(a,x) a.registerWords(_wordlist_##x)

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
    int opcode;
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
        uint32_t fixup; //!< ID of a saved fixup block
    }d;
};


/// an entry in the closure map - the variable in this slot should be obtained
/// from the variable 'parent' in the parent context - isLocalInParent indicates
/// whether it should come from the local variables or the closure variables.

struct ClosureMapEntry {
    int parent;
    bool isLocalInParent;
};


#define MAXLOCALS	32

/// a compilation context - we have a stack of these so that we can
/// do lambdas

class CompileContext {
    int compileCt; //!< words so far in compile buffer
    Instruction compileBuf[1024]; //!< compile buffer
    Stack<int,8> cstack; //!< location stack for loops etc.
    Stack<int,8> leaveListStack; //!< stack of leave instruction lists - linked through the d.i field, this is a list of OP_LEAVE etc. which must be resolved when a loop ends.
    
    char localTokens[MAXLOCALS][64]; //!< locals in the word currently being defined
    int localTokenCt; //!< number of locals in the word currently being defined
    int paramCt; //!< number of locals which are parameters; set by the compileLocalAndParams() method
    
    /// the closure map - which maps the closure variables onto locals or closures
    /// in the containing context.
    
    ClosureMapEntry closureMap[MAXLOCALS]; 
    char closureMapNames[MAXLOCALS][64]; //!< names of variable in the container context
    int closureMapCt;
    int addClosureMapEntry(const char *name, int parentidx, bool isLocalInParent){
        strcpy(closureMapNames[closureMapCt],name);
        closureMap[closureMapCt].isLocalInParent = isLocalInParent;
        closureMap[closureMapCt].parent = parentidx;
        return closureMapCt++;
    }
    
    CompileContext *parent; //!< pointer to containing context or NULL, set up by pushCompileContext
    int leaveListHead; //!< the head of a leave list - the index of the first OP_LEAVE etc. instruction, or -1.
    const char *spec; //!< specification string
    
public:
    
    CompileContext(){
        spec=NULL;
        reset(NULL);
    }
    
    
    /// reset a new compile context and set the containing context.
    void reset(CompileContext *p){
        parent = p;
        compileCt=0;
        compileBuf[0].opcode=1; //OP_END
        paramCt=0;
        localTokenCt=0;
        closureMapCt=0;
        if(spec){
            free((void *)spec);
            spec=NULL;
        }
        leaveListHead = -1;
        cstack.clear();
    }
    
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
    /// return a newly allocated copy of the closure map
    ClosureMapEntry *copyClosureMap(){
        ClosureMapEntry *map = new ClosureMapEntry[closureMapCt];
        memcpy(map,closureMap,closureMapCt*sizeof(ClosureMapEntry));
        return map;
    }
    /// return the size of the closure map
    int getClosureMapSize(){
        return closureMapCt;
    }
    
                                           
    int addLocalToken(const char *s){
        int t = localTokenCt;
        if(localTokenCt==MAXLOCALS)
            throw RUNT("too many local tokens");
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
    
    
    /// attempt to create a closure for the variable s in the context above.
    /// We do this by asking the parent context for the local ID of that variable,
    /// or if it can't find it to create a closure itself. This will recurse up
    /// to the root.
    
    int findOrAttemptCreateClosure(const char *s){
        int i;
        
        if(!parent)return -1; // no closure if there's no parent
        
        // do we already have a closure?
        for(int i=0;i<closureMapCt;i++){
            if(!strcmp(closureMapNames[i],s))
                return i;
        }
        
        
        if((i=parent->getLocalToken(s))>=0){
            // we've found a local in the parent, so we can create
            // a closure which refers to a local
            return addClosureMapEntry(s,i,true); // and return the new closure ID
        }
        
        
        // Find the local in the parent, so we can create a closure
        if((i=parent->findOrAttemptCreateClosure(s))>=0){
            // here, we've found a closure which refers to a closure in the parent
            return addClosureMapEntry(s,i,false);
        }
        return -1;
    }
    
    /// return the index of a local token, or -1 if it's not in the table
    int getLocalToken(const char *s){
        for(int i=0;i<localTokenCt;i++){
            if(!strcmp(s,localTokens[i]))
                return i;
        }
        return -1;
    }
    
    Instruction *compile(int opcode){
        if(compileCt==1024)
            throw SyntaxException("word too long");
        Instruction *i = compileBuf+compileCt;
        compileCt++;
        i->opcode = opcode;
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
/// a count of how many of those should be popped off the stack (i.e. are parameters),
/// and a optional closure map, which indicates how a closure table will be created
/// when the OP_LITERALCODE to which this is attached is run.

/// Both CodeBlocks and Closures can be called with OP_CALL.

struct CodeBlock {
    
    void setFromContext(CompileContext *con){
        spec = NULL;
        ip = con->copyInstructions();
        locals = con->getLocalCount();
        params = con->getParamCount();
        size = con->getCodeSize();
        closureMapCt = con->getClosureMapSize();
        if(closureMapCt)
            closureMap = con->copyClosureMap();
        else
            closureMap = NULL;
    }
    
    void clear(){
        if(ip) delete[] ip;
        if(closureMap) delete[] closureMap;
        if(spec)free((void *)spec);
        ip=NULL;
        closureMap=NULL;
        spec=NULL;
    }
    
    
    void save(Serialiser *ser) const;
    CodeBlock(Serialiser *ser);
    
    CodeBlock(CompileContext *con){
        setFromContext(con);
    }
    
    CodeBlock(const Instruction *i){
        ip=i;
        spec = NULL;
        size=0;
        locals = 0;
        params = 0;
        closureMapCt=0;
        closureMap=NULL;
    }
    
    const Instruction *ip; //!< the instructions themselves, must be delete[]ed
    const char *spec; //!< the specification --- null by default, but if present must be freed
    int size;
    int locals,params;
    ClosureMapEntry *closureMap;
    int closureMapCt;
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
    
    // localct is count of locals AND parms, parmct is just parms.
    void allocLocalsAndPopParams(int localct,int parmct,Stack<Value,32> *stack){
        //        printf("locals : %d, params: %d\n",localct,parmct);
        next += localct;
        for(int i=0;i<parmct;i++){
            vars[base+(parmct-i)-1].copy(stack->popptr());
            //            printf("Parameter %d = variable %d = %s\n",
            //                   i,base+(parmct-i)-1,vars[base+(parmct-i)-1].getAsString());
        }
    }
    
    Value *get(int n){
        return vars+base+n;
    }
};

/// a module is a set of native functions grouped by functionality
struct Module {
    /// the name of the module - may be "" if the module was registered
    /// directly with registerFunc() with no optional module name
    const char *module;
    
    /// the functions
    StringMap<NativeFunc> funcs;
    /// and the properties
    StringMap<Property *> props; 
};

class Angort {
    friend class Serialiser;
    friend struct CodeBlock; // for serialisation
private:
    Stack<const Instruction *,32> rstack;
    Stack<Value*,32> closureStack; //<! parallels the return stack, carries the closure table
    /// also parallels the return stack, used for closures-during-use
    /// (see closures.ang test cases)
    Stack<GarbageCollected *,32> gcrstack;
    
    Stack<int,32> cstack;
    Stack<Value,8> loopIterStack; // stack of loop iterators
    
    StringMap<Module *> modules; //!< a list of modules
    
    ContiguousValueSet consts;
    ContiguousValueSet globals;
    ContiguousValueSet words;
    
    Stack<CompileContext,4> contextStack;
    VarStack locals;
    Value *closureTable; //!< the current closure table
    
    /// the functions, duplicates of the module entries
    StringMap<NativeFunc> funcs;
    StringMap<const char *> funcSpecs;
    /// and the properties, duplicates of the module entries
    StringMap<Property *> props; 
    StringMap<const char *> propSpecs;
    
    /// the current compile context
    CompileContext *context;
    
    Tokeniser tok;
    const Instruction *ip,*debugwordbase;
    
    /// define a word from a context
    void define(const char *name,class CompileContext *cb);
    /// define a word just from a list of instructions
    void define(const char *name,Instruction *i);
    
    char defineName[256]; //!< name of the word we're defining
    
    char lastLine[1024]; //!< last line read
    
    /// add a new instruction to the current compile context
    Instruction *compile(int opcode){
        return context->compile(opcode);
    }
    
    /// push the current compile context onto the stack and clear the new one
    void pushCompileContext(){
        CompileContext *p = context;
        context = contextStack.pushptr();
        context->reset(p);
    }
    
    /// pop a context off the stack, ready to carry on where we left off.
    /// We also return a pointer to the previous context, so we can reference
    /// its code. We do not reset it.
    CompileContext *popCompileContext(){
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
    
    /// this will push the locals stack and then
    /// resolve any closures in the value,
    /// and finally push the rstack. The new IP
    /// is returned, and the old one is passed in
    /// (which could be NULL for top level).
    const Instruction *call(const Value *v, const Instruction *returnip);
    
    
    /// find a global or create one if it doesn't exist;
    /// used for autoglobals (!! and ??)
    int findOrCreateGlobal(const char *name){
        int i = globals.get(name);
        if(i<0)
            i = globals.add(name);
        return i;
    }
    
    /// called at the end of a block of code,
    /// or by emergency stop invocation. Returns
    /// the new IP or NULL.
    const Instruction *ret();
    
public:
    Stack<Value,32>stack;
    bool emergencyStop;
    bool defining;
    bool debug;
    /// make assertions print statements even when they pass just fine,
    /// used in testing.
    bool assertDebug;
    /// print each line we parse
    bool printLines;
    
    /// only valid in feedFile()
    int lineNumber;
    
    /// return line number, only valid in feedFile()
    int getLineNumber(){
        return lineNumber;
    }
        
    /// dump the stack to stdout
    void dumpStack(const char *s);
    
    
    /// get the top iterator on the iterator stack (or the nth)
    IteratorObject *getTopIterator(int i=0){
        return loopIterStack.peekptr(i)->v.iter;
    }
    
    /// visit the tree of all globally-accessible data,
    /// used in serialisation
    void visitGlobalData(ValueVisitor *visitor);
    
    /// clear the entire system
    void clear(){
        stack.clear();
        words.clear();
        consts.clear();
        globals.clear();
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
    
    int popInt(){
        return Types::tInteger->get(popval());
    }
    float popFloat(){
        return Types::tFloat->get(popval());
    }
    const char *popString(char *buf,int len){
        return popval()->toString(buf,len);
    }
    
    Module *findOrCreateModule(const char *n){
        Module *m;
        
        if(!n)n="";
        if(!modules.find(n)){
            m = new Module();
            modules.set(n,m);
        } else
            m = modules.found();
        return m;
    }
    
    void registerFunc(const char *name,NativeFunc f,const char *module=NULL,const char *spec=NULL){
        Module *m = findOrCreateModule(module);
        m->funcs.set(name,f);
        funcSpecs.set(name,spec);
        funcs.set(name,f);
    }
    
    void registerProperty(const char *name, Property *p, const char *module=NULL,const char *spec=NULL){
        Module *m = findOrCreateModule(module);
        m->props.set(name,p);
        propSpecs.set(name,spec);
        props.set(name,p);
    }
    
    /// register a whole bunch of words
    void registerWords(AngortWordDef *first){
        while(first->name){
            registerFunc(first->name,first->f,first->modname,first->desc);
            first++;
        }
    }
    
    
    Angort();
    
    /// just compile a string into a set of instructions
    const Instruction *compile(const char *s);
    
    /// feed a string into the interpreter
    void feed(const char *s);
    /// feed a whole file; will print a message and return
    /// false if there is a problem, or (and this is the default)
    /// throw an exception
    bool fileFeed(const char *name,bool rethrow=true);
    
    /// run until OP_END received and no return stack
    void run(const Instruction *ip);
    
    /// used to run a codeblock or closure - works by doing call() and then run() until exit.
    /// Will not push the closuretable or return stacks.
    void runValue(const Value *v);
    
    /// disassemble a named word
    void disasm(const char *name);
    
    /// list all words, globals and consts
    void list();
    
    /// get the spec string for a word or native
    const char *getSpec(const char *s);
    
    /// load an image file
    void loadImage(const char *name);
    
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
    
};



#endif /* __ANGORT_H */
