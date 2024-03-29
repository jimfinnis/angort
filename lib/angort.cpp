/**
 * @file 
 *
 * 
 * @author $Author$
 * @date $Date$
 */


// semantic versioning: (incs on backcompat breaking features).
//                      (incs on backcompat retaining features).
//                      (incs on bug fixing patches)

#define ANGORT_VERSION "4.16.0"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

#include "angort.h"
#define DEFOPCODENAMES 1
#include "opcodes.h"
#include "tokens.h"
#include "hash.h"
#include "cycle.h"

/// this special hash key is a "symbol" used for catch blocks
/// that catch all exceptions.
#define CATCHALLKEY 0xdeadbeef

extern angort::LibraryDef LIBNAME(coll),LIBNAME(string),LIBNAME(std),
LIBNAME(math),LIBNAME(env),LIBNAME(future),LIBNAME(deprecated);


#if ANGORT_POSIXLOCKS
extern angort::LibraryDef LIBNAME(thread);
#endif

namespace angort {

bool hasLocking(){
#if ANGORT_POSIXLOCKS
    return true;
#else
    return false;
#endif
}

Lockable globalLock("global");

const char* Angort::getVersion(){
    return ANGORT_VERSION;
}

Runtime::Runtime(Angort *angort,const char *_name){
    static int idcounter=0;
    id = idcounter++;
    ang = angort;
    name = _name;
    thread = NULL; // will get changed if we're being created in a thread
    ip = NULL;
    traceOnException=true;
    outputStream = stdout;
    debuggerNextIP=false;
    debuggerStepping=false;
    rstack.setName("return");
    loopIterStack.setName("loop iterator");
    stack.setName("main");
    catchstack.setName("catchstack");
    trace=false;
    emergencyStop=false;
    assertDebug=false;
    assertNegated=false;
    loopIterCt=0;
    autoCycleCount = AUTOGCINTERVAL;
    
    long t;
    time(&t);
    srand48_r(t+id*17,&rnd);
}

Runtime::~Runtime(){
    endredir();
}

void Runtime::gc(){
    WriteLock lock = WL(&globalLock);
    GarbageCollected::gc();
}    

Angort::Angort() {
    {
        WriteLock lock=WL(&names);
        
        Types::createTypes();
        // create and set default namespace
        stdNamespace = names.create("std");
        // import everything from it
        names.import(stdNamespace,NULL);
        // and that's the namespace we're working in
        names.push(stdNamespace);
    }
    showinit = true;
    lineNumber=1;
    running = true;
    isSkipping = false;
    inCompileIf = false;
    
    // no debugger by default; CLI sets this up.
    debuggerHook = NULL;
    
    contextStack.setName("context");
    
    // initialise the default runtime.
    run = new Runtime(this,"default");
    
    // initialise the search path to the environment variable ANGORTPATH
    // if it exists, otherwise to the default.
    const char *spenv = getenv("ANGORTPATH");
    searchPath=strdup(spenv?spenv:DEFAULTSEARCHPATH);
    
    libs = new ArrayList<LibraryDef*>(8);
    tok.init();
    tok.setname("<stdin>");
    tok.settokens(tokens);
    tok.setcommentlinesequence("#");
    
    registerLibrary(&LIBNAME(std)); // already imported by default.
    registerLibrary(&LIBNAME(coll),true);
    registerLibrary(&LIBNAME(string),true);
    registerLibrary(&LIBNAME(math),true);
    registerLibrary(&LIBNAME(env),true);
    
    // libraries which are not imported by default
#if ANGORT_POSIXLOCKS
    registerLibrary(&LIBNAME(thread),false);
#endif    
    
    // future and deprecated are not imported
    registerLibrary(&LIBNAME(future),false);
    registerLibrary(&LIBNAME(deprecated),false);
    
    
    tokeniserTrace=false;
    
    // now the standard package has been imported, set up the
    // user package into which their words are defined.
    
    {
        WriteLock lock=WL(&names);
        int userNamespace = names.create("user");
        names.import(userNamespace,NULL);
        names.push(userNamespace);
    }
    
    printLines=false;
    wordValIdx=-1;
    barewords=false;
    autoCycleInterval = AUTOGCINTERVAL;
    hereDocString = hereDocEndString = NULL;
    
    /// create the default, root compilation context
    context = contextStack.pushptr();
    context->ang = this;
    context->reset(NULL,&tok);
    
    acList = new ArrayList<const char *>(128);
}

Angort::~Angort(){
    shutdown();
}

void Angort::importAllFuture(){
    WriteLock lock=WL(&names);
    Namespace *ns = names.getSpaceByName("future");
    names.import(ns->idx,NULL);
    
}

void Angort::importAllDeprecated(){
    WriteLock lock=WL(&names);
    Namespace *ns = names.getSpaceByName("deprecated");
    names.import(ns->idx,NULL);
}

void CompileContext::dump(){
    printf("CompileContext dump for context %p with codeblock %p, closure block size %d, closure table size %d\n  Locals\n",
           this,cb,cb->closureBlockSize,cb->closureTableSize);
    printf("  Closure count %d, Param count %d\n",closureCt,paramCt);
    printf("     %20s %8s %s","name","closed?","localindex\n");
    for(int i=0;i<localTokenCt;i++){
        printf("     %20s %8s %d\n",
               localTokens[i],
               (localsClosed&(1<<i))?"C":" ",
               localIndices[i]);
    }
    printf("Disassembly:\n");
    if(ang){
        ang->disasm(cb);
    } else {
        printf("<no angort ptr>\n");
    }
}


void Angort::shutdown(){
    if(running){
        ArrayListIterator<LibraryDef *>iter(libs);
        
        for(iter.first();!iter.isDone();iter.next()){
            LibraryDef *lib = *(iter.current());
            NativeFunc shutdownFunc = lib->shutdownfunc;
            if(shutdownFunc)
                (*shutdownFunc)(run);
        }
        
        Type::clearList();
        SymbolType::deleteAll();
        running = false;
    }
}

void Runtime::showop(const Instruction *ip,int indent,const Instruction *base,
                     const Instruction *curr){
    ReadLock lock(&ang->names);
    if(!base)base=wordbase;
    char buf[128],indentStr[32];
    Value tmp;
    indent = (indent<31)?indent:31;
    if(indent>0)
        memset(indentStr,' ',indent);
    indentStr[indent]=0;
    
    // print the start of the string
    printf("%s%s%s %3d %8p [%s:%d] : %04d : %s (%d) ",indentStr,
#if SOURCEDATA
           ip->brk ? "B " : "  ",
#else
           "  ",
#endif
           ip == curr ? "* " : "  ",
           id,
           base,
#if SOURCEDATA
           ip->file,ip->line,
#else
           "?",0,
#endif
           (int)(ip-base),
           opcodenames[ip->opcode],
           ip->opcode);
    
    // print extra data at the end of the line
    switch(ip->opcode){
    case OP_FUNC:
        Types::tNative->set(&tmp,ip->d.func);
        printf(" (%s)",ang->names.getNameByValue(&tmp,buf,128));
        break;
    case OP_JUMP:
    case OP_LEAVE:
    case OP_IF:
    case OP_ITERLEAVEIFDONE:
        printf("(offset %d)",ip->d.i);
        break;
    case OP_GLOBALDO:
    case OP_GLOBALSET:
    case OP_GLOBALGET:
    case OP_GLOBALINC:
    case OP_GLOBALDEC:
        {
            char buf[256];
            ang->names.getFQN(ip->d.i,buf,256);
            printf("(%s)",buf);
        }
        break;
    case OP_CLOSURESET:
    case OP_CLOSUREGET:
    case OP_CLOSUREDEC:
    case OP_CLOSUREINC:
    case OP_LOCALSET:
    case OP_LOCALGET:
    case OP_LOCALINC:
    case OP_LOCALDEC:
        printf("(idx %d)",ip->d.i);break;
    case OP_PROPSET:
    case OP_PROPGET:
        Types::tProp->set(&tmp,ip->d.prop);
        printf(" (name %s)",ang->names.getNameByValue(&tmp,buf,128));
        break;
    case OP_LITERALSTRING:
        printf("(%s)",ip->d.s);
        break;
    case OP_LITERALINT:
        printf("(%d)",ip->d.i);
        break;
    case OP_LITERALFLOAT:
        printf("(%f)",ip->d.f);
        break;
    case OP_LITDOUBLE:
        printf("(%f)",ip->d.df);
        break;
    case OP_LITLONG:
        printf("(%ld)",ip->d.l);
        break;
    case OP_LITERALSYMB:
    case OP_HASHGETSYMB:
    case OP_HASHSETSYMB:
        {
            ReadLock l(Types::tSymbol);
            printf("(%d:%s)",ip->d.i,
                   Types::tSymbol->getString(ip->d.i));
            break;
        }
    case OP_LITERALCODE:
        printf("code follows:\n"); // end the line
        ang->disasm(ip->d.cb, indent+4);
        break;
    default:break;
    }
    tmp.clr();
    
}

const Instruction *Runtime::call(const Value *a,const Instruction *returnip){
    const CodeBlock *cb;
    const Type *t;
    
    if(a->isNone())return returnip; // NONE does nothing when called
    
    t=a->getType();
    
    // if it's a native C++ function, just call it
    if(t==Types::tNative){
        (*a->v.native)(this);
        return returnip;
    }
    
    locals.push(); // switch to new locals frame
    // get the codeblock, check the type, and in
    // the case of a closure, bind the closed locals
    
    Closure *clos;
    if(t==Types::tCode){
        clos = NULL;
        cb = a->v.cb;
    } else if(t == Types::tClosure){
        clos = a->v.closure;
        cb = clos->cb;
    } else {
        throw RUNT(EX_NOTFUNC,"").set("attempt to 'call' something that isn't code, it's a %s",t->name);
    }
    
    if(!cb->ip)
        throw RUNT(EX_DEFCALL,"call to a word with a deferred definition");
    
#if DEBCLOSURES
    printf("Locals = %d of which closures = %d\n",cb->locals,cb->closureBlockSize);
    printf("Allocating %d stack spaces\n",cb->locals - cb->closureBlockSize);
#endif
    // allocate true locals (stack locals)
    locals.alloc(cb->locals - cb->closureBlockSize);
    
    // now pop parameters, in reverse order, by
    // peeking them and then dropping the whole
    // lot in one go.
    
    uint8_t *pidx = cb->paramIndices;
    
    Value tmpval;
    for(int i=0;i<cb->params;i++,pidx++){
        Value *paramval = stack.peekptr((cb->params-1)-i);
        
        Type *tp = cb->paramTypes[i];
        Value *valptr=paramval; // the value we'll actually store
        if(tp){
            if(tp == Types::tNumber || tp == Types::tStringStrict){
                if(paramval->t->supertype != tp){
                    throw RUNT(EX_BADPARAM,"").set("Type mismatch: argument %d is %s, expected a %s",
                                                   i,paramval->t->name,tp->name);
                }
            } else if(tp != paramval->t){
                // type check with possible conversion
                try {
                    tp->toSelf(&tmpval,paramval);
                } catch(BadConversionException e){
                    throw RUNT(EX_BADPARAM,"").set("Type mismatch: argument %d is %s, expected %s",
                                                   i,paramval->t->name,tp->name);
                }
                valptr = &tmpval;
            }
        }                          
        
        
        
        if(cb->localsClosed & (1<<i)){
            //                        printf("Param %d is closed: %s, into closure %d\n",i,paramval->toString().get(),*pidx);
            clos->map[*pidx]->copy(valptr);
        } else {
            //                        printf("Param %d is open: %s, into local %d\n",i,paramval->toString().get(),*pidx);
            locals.store(*pidx,valptr);
        }
    }
    stack.drop(cb->params);
    //        if(clos)clos->show("VarStorePostParams");
    
    
    // do the push
    Frame *f = rstack.pushptr();
    catchstack.pushptr()->clear();
    // This might be null, and in that case we ignore it when we pop it.
    f->ip = returnip;
    f->base = wordbase;
    f->rec.copy(a); // and also stack the value itself
    // stack the current level's closure
    f->clos.copy(&currClosure);
    if(clos)
        Types::tClosure->set(&currClosure,clos);
    else
        currClosure.clr();
    
#if DEBCLOSURES
    printf("PUSHED closure %s\n",currClosure.toString().get());
#endif
    
    f->loopIterCt=loopIterCt;
    loopIterCt=0;
    
    // if the closure has a stored IP (due to a yield) then start
    // from there, otherwise start from the codeblock's beginning.
    struct Instruction *ip;
    if(clos && clos->ip)
        ip=clos->ip;
    else
        ip = (Instruction *)cb->ip;
    
    wordbase = ip;
    return ip;
}

void CodeBlock::setFromContext(CompileContext *con){
    ip = con->copyInstructions();
    locals = con->getLocalCount();
    params = con->getParamCount();
    size = con->getCodeSize();
    closureTable = con->makeClosureTable(&closureTableSize);
    closureBlockSize = con->closureCt;
    localsClosed = con->localsClosed;
    
    
    paramIndices = new uint8_t[params];
    paramTypes = new Type * [params];
    
    for(int i=0;i<params;i++){
        paramIndices[i] = con->getLocalIndex(i);
        paramTypes[i] = con->getLocalType(i);
    }
    
    used=true;
}

void Runtime::runValue(const Value *v){
    if(!emergencyStop){
        const Instruction *oldbase=wordbase;
        const Instruction *previp = ip;
        const Instruction *fip=call(v,NULL);
        if(fip)run(fip);
        wordbase=oldbase;
        ip=previp;
    }
    //    locals.pop(); 
}

void Runtime::dumpStack(const char *s){
    printf("Stack dump for %s\n",s);
    for(int i=0;i<stack.ct;i++){
        const StringBuffer &b = stack.peekptr(i)->toString();
        printf("%3d  %s\n",i,b.get());
    }
}

void Runtime::ret()
{
    // pop the appropriate number of iterator frames
    try{
        for(int i=0;i<loopIterCt;i++){
            loopIterStack.popptr()->clr();
        }
        loopIterCt=0;
    }catch(StackUnderflowException e){
        throw SyntaxException("loop iterator mismatch\n");
    }
    
    // TODO sanity check and pop of closure stack
    if(rstack.isempty()){
        ip=NULL;
    } else {
        Frame *f = rstack.popptr();
        catchstack.pop();
        ip = f->ip;
        wordbase = f->base;
        f->rec.clr();
        // copy the current closure back in
        
        //        printf("RETURNING. CurrClosure currently %s\n",
        //               currClosure.toString().get());
        
        currClosure.copy(&f->clos);
        
        //        printf("           CurrClosure now       %s\n",
        //               currClosure.toString().get());
        
        
        f->clos.clr();
        loopIterCt = f->loopIterCt;
        locals.pop();
    }
}

bool Runtime::throwAngortException(int symbol, Value *data){
    storeTrace(); // store a trace to print if we need to
    
    // we go up the exception stack, inner (intrafunction) stack
    // first - if that doesn't find a handler, we pop the outer
    // stack by performing a return.
    
    while(!rstack.isempty()){
        // get the current intrafunction catch stack
        Stack<IntKeyedHash<int>*,4> *cs = catchstack.peekptr();
        // now go through it
        while(!cs->isempty()){
            IntKeyedHash<int> *h = cs->pop();
            int *offset = h->ffind(symbol);
            if(offset){
                // FOUND IT - deal with it and return, stacking
                // the value and the exception ID.
                pushval()->copy(data);
                Types::tSymbol->set(pushval(),symbol);
                ip = wordbase+*offset;
                return true;
            }
            // failing that, look for a catch-all.
            offset = h->ffind(CATCHALLKEY);
            if(offset){
                // FOUND IT - deal with it and return, stacking
                // the value and the exception ID.
                pushval()->copy(data);
                Types::tSymbol->set(pushval(),symbol);
                ip = wordbase+*offset;
                return true;
            }
            // failing THAT, pop another try-catch set off.
        }
        // didn't find it in the intrafunction stack, need
        // to return from this function and try again.
        ret();
    }
    // no handler found, abort
    return false;
}

void Runtime::run(const Instruction *startip){
    ip=startip;
    
    Value *a, *b, *c;
    wordbase = ip;
    const CodeBlock *cb;
    // push initial catchstack
    catchstack.pushptr()->clear();
    
    try {
        for(;;){
            try {
                if(emergencyStop){
                    ret();
                    if(!ip)
                        goto leaverun;
                }
                
                int opcode = ip->opcode;
                if(trace){
                    showop(ip,0,wordbase);
                    printf(" ST [%d] : ",stack.ct);
                    for(int i=0;i<stack.ct;i++){
                        const StringBuffer &sb = stack.peekptr(i)->toString();
                        printf("%s ",sb.get());
                    }
                    printf("\n");
                }
                // only the default thread does cycle GC
                if(!id && ang->autoCycleInterval>0 && !--autoCycleCount){
                    autoCycleCount = ang->autoCycleInterval;
                    gc();
                }
#if SOURCEDATA
                // breakpoint set on instruction, invoke debugger. This somewhat
                // clumsy logic is here because SOURCEDATA might conceivably be
                // false.
                if(ip->brk)debuggerNextIP=true;
#endif  
                if(debuggerNextIP && ang->debuggerHook){
                    if(!debuggerStepping)
                        debuggerNextIP = false;
                    (*ang->debuggerHook)(this);
                }
                
                switch(opcode){
                case OP_EQUALS:      case OP_ADD:            case OP_MUL:
                case OP_DIV:         case OP_SUB:            case OP_NEQUALS:
                case OP_AND:         case OP_OR:             case OP_GT:
                case OP_LT:          case OP_MOD:            case OP_CMP:
                case OP_LE:          case OP_GE:
                    b = popval();
                    a = popval();
                    binop(a,b,opcode);
                    ip++;
                    break;
                case OP_NOP:
                    ip++;
                    break;
                case OP_INC:
                    stack.peekptr()->increment(ip->d.i);
                    ip++;
                    break;
                case OP_LITERALINT:
                    pushInt(ip->d.i);
                    ip++;
                    break;
                case OP_LITLONG:
                    pushLong(ip->d.l);
                    ip++;
                    break;
                case OP_LITDOUBLE:
                    pushDouble(ip->d.df);
                    ip++;
                    break;
                case OP_LITERALFLOAT:
                    pushFloat(ip->d.f);
                    ip++;
                    break;
                case OP_LITERALSTRING:
                    pushString(ip->d.s);
                    ip++;
                    break;
                case OP_LITERALCODE:{
                    cb = ip->d.cb;
                    a = stack.pushptr();
                    // as in globaldo, here we construct a 
                    // closure if required and stack that instead.
                    if(cb->closureBlockSize || cb->closureTableSize ){
                        //                        printf("OP_LITERALCODE running - creating a closure. Blocksize is %d, tablesize is %d\n",
                        //                               cb->closureBlockSize,cb->closureTableSize);
                        Closure *cl = new Closure(currClosure.v.closure); // 1st stage of setup
                        Types::tClosure->set(a,cl);
                        a->v.closure->init(cb); // 2nd stage of setup
                    } else
                        Types::tCode->set(a,cb);
                    ip++;
                }
                    break;
                case OP_CLOSUREGET:
                    if(!currClosure.t)throw WTF;
                    else if(currClosure.t == Types::tNone)
                        throw RUNT(EX_SYNTAX,"current closure is \"none\" : attempt to use local in constexpr?");
                    else if(currClosure.t != Types::tClosure)
                        throw RUNT(EX_WTF,"").set("weird type in closure: %s",currClosure.t->name);
                    a = currClosure.v.closure->map[ip->d.i];
#if DEBCLOSURES
                    currClosure.v.closure->show("VarGet");
#endif
                    stack.pushptr()->copy(a);
                    ip++;
                    break;
                case OP_CLOSURESET:
                    if(currClosure.t != Types::tClosure)throw WTF;
#if DEBCLOSURES
                    currClosure.v.closure->show("VarSet");
#endif
                    a = currClosure.v.closure->map[ip->d.i];
                    a->copy(stack.popptr());
                    ip++;
                    break;
                case OP_CLOSUREINC:
                    if(currClosure.t != Types::tClosure)throw WTF;
                    currClosure.v.closure->map[ip->d.i]->increment(1);
                    ip++;
                    break;
                case OP_CLOSUREDEC:
                    if(currClosure.t != Types::tClosure)throw WTF;
                    currClosure.v.closure->map[ip->d.i]->increment(-1);
                    ip++;
                    break;
                case OP_GLOBALSET:
                    {
                        WriteLock lock=WL(&ang->names);
                        // SNARK - combine with consts
                        a = popval();
                        ang->names.getVal(ip->d.i)->copy(a);
                        ip++;
                    }
                    break;
                case OP_GLOBALINC:
                    {
                        WriteLock lock=WL(&ang->names);
                        ang->names.getVal(ip->d.i)->increment(1);
                        ip++;
                    }
                    break;
                case OP_GLOBALDEC:
                    {
                        WriteLock lock=WL(&ang->names);
                        ang->names.getVal(ip->d.i)->increment(-1);
                        ip++;
                    }
                    break;
                case OP_PROPGET:
                    ip->d.prop->preGet(); 
                    a = stack.pushptr();
                    a->copy(&ip->d.prop->v);
                    ip->d.prop->postGet(); // for completeness, really
                    ip++;
                    break;
                case OP_PROPSET:
                    ip->d.prop->preSet(); // so we can pick up extra params
                    a = popval();
                    ip->d.prop->v.copy(a);
                    ip->d.prop->postSet();
                    ip++;
                    break;
                case OP_FUNC:
                    try {
                        (*ip->d.func)(this);
                    } catch(const char *strex){
                        strex=strdup(strex);// buh???
                        throw RUNT(EX_NATIVE,strex);
                    }
                    ip++;
                    break;
                case OP_GLOBALDO:
                    {
                        ReadLock lock(&ang->names);
                        a = ang->names.getVal(ip->d.i);
                        if(a->t->isCallable()){
                            Closure *clos;
                            Value vv;
                            // here, we construct a closure block for the global if
                            // required. This results in a new value being created which
                            // goes into the frame.
                            if(a->t == Types::tCode){
                                const CodeBlock *cb = a->v.cb;
                                if(cb->closureBlockSize || cb->closureTableSize){
                                    //                        printf("OP_GLOBALDO running to call a closure - creating the closure. Blocksize is %d, tablesize is %d\n",
                                    //                               cb->closureBlockSize,cb->closureTableSize);
                                    clos = new Closure(NULL); // 1st stage of setup
                                    Types::tClosure->set(&vv,clos);
                                    a = &vv;
                                    a->v.closure->init(cb);
                                    
                                    /* This earlier code inadvertently set currClosure too soon,
                                     * before it gets pushed in call(), thus resulting in an incorrect
                                     * closure being popped in ret(). The above code should be correct.
                                     * JCF 07/12/14
                                       clos = new Closure(NULL); // 1st stage of setup
                                       // if a closure was made, we store it in the current
                                       // frame.
                                       Types::tClosure->set(&currClosure,clos);
                                       a = &currClosure; // and this is the value we call.
                                       a->v.closure->init(cb); // 2nd stage of setup
                                     */
                                }
                            }
                            // we call this value.
                            ip = call(a,ip+1);
                        } else if(a->t == Types::tNone) {
                            // if it's NONE we drop it
                            ip++;
                        } else {
                            // if not callable we just stack it.
                            b = stack.pushptr();
                            b->copy(a);
                            ip++;
                        }
                    }
                    break;
                case OP_GLOBALGET:
                    {
                        ReadLock lock(&ang->names);
                        // like the above but does not run a codeblock
                        a = ang->names.getVal(ip->d.i);
                        b = stack.pushptr();
                        b->copy(a);
                        ip++;
                    }
                    break;
                case OP_CALL:
                    // easy as this - pass in the value
                    // and the return ip, get the new ip
                    // out.
                    ip=call(popval(),ip+1); // this JUST CHANGES THE IP AND STACKS STUFF.
                    break;
                case OP_SELF:
                    stack.pushptr()->copy(&(rstack.peekptr()->rec));
                    ip++;
                    break;
                case OP_RECURSE:
                    a = &(rstack.peekptr()->rec);
                    ip=call(a,ip+1);
                    break;
                case OP_END:
                case OP_STOP:
                    ret();
                    if(!ip)
                        goto leaverun;
                    break;
                case OP_YIELD:
                    // it's a closure, so stash the next IP into
                    // the closure.
                    currClosure.v.closure->ip = (Instruction *)ip+1;
                    ret();
                    if(!ip)goto leaverun;
                    break;
                case OP_IF:
                    if(popBool())
                        ip++;
                    else
                        ip+=ip->d.i;
                    break;
                case OP_DUP:
                    a = stack.peekptr();
                    b = stack.pushptr();
                    b->copy(a);
                    ip++;
                    break;
                case OP_OVER:
                    a = stack.peekptr(1);
                    b = stack.pushptr();
                    b->copy(a);
                    ip++;
                    break;
                case OP_LEAVE:
                    loopIterStack.popptr()->clr();
                    loopIterCt--;
                    // fall through
                case OP_JUMP:
                    ip+=ip->d.i;
                    break;
                case OP_IFLEAVE:
                    if(popBool()){
                        loopIterStack.popptr()->clr();
                        loopIterCt--;
                        ip+=ip->d.i;
                    } else
                        ip++;
                    break;
                case OP_LOOPSTART:
                    // start of an infinite loop, so push a None iterator
                    a = loopIterStack.pushptr();
                    loopIterCt++;
                    a->clr();
                    ip++;
                    break;
                case OP_ITERSTART:{
                    a = stack.popptr(); // the iterable object
                    // we make an iterator and push it onto the iterator stack
                    b = loopIterStack.pushptr();
                    loopIterCt++;
                    a->t->createIterator(b,a);
                    ip++;
                    break;
                }
                case OP_ITERLEAVEIFDONE:{
                    a = loopIterStack.peekptr(); // the iterator object
                    Iterator<Value *> *iter = a->v.iter->iterator;
                    if(iter->isDone()){
                        // and pop the iterator off and clear it, for GC.
                        loopIterStack.popptr()->clr();
                        loopIterCt--;
                        // and jump out
                        ip += ip->d.i;
                    } else {
                        // stash the current away, we're about to change it
                        // because we need to put the 'next' in here.
                        a->v.iter->current->copy(iter->current());
                        iter->next();
                        ip++;
                    }
                    break;
                }
                case OP_NOT:
                    a = stack.peekptr();
                    Types::tInteger->set(a,a->toBool()?0:1);
                    ip++;
                    break;
                case OP_SWAP:
                    {
                        a = stack.peekptr(0);
                        b = stack.peekptr(1);
                        Value t;
                        t.copy(b);
                        b->copy(a);
                        a->copy(&t);
                        ip++;
                        break;
                    }
                case OP_DROP:
                    popval();
                    ip++;
                    break;
                case OP_LOCALSET:
                    a = stack.popptr();
                    b = locals.get(ip->d.i);
                    b->copy(a);
                    ip++;
                    break;
                case OP_LOCALGET:
                    a = stack.pushptr();
                    b = locals.get(ip->d.i);
                    a->copy(b);
                    ip++;
                    break;
                case OP_LOCALINC:
                    locals.get(ip->d.i)->increment(1);
                    ip++;
                    break;
                case OP_LOCALDEC:
                    locals.get(ip->d.i)->increment(-1);
                    ip++;
                    break;
                case OP_DOT:{
                    a = popval();
                    const StringBuffer &sb = a->toString();
                    fputs(sb.get(),outputStream);
                    fputc('\n',outputStream);
                }
                    ip++;
                    break;
                case OP_NEWLIST:
                    Types::tList->set(pushval());
                    ip++;
                    break;
                case OP_NEWHASH:
                    Types::tHash->set(pushval());
                    ip++;
                    break;
                case OP_HASHGETSYMB:
                    {
                        Value t;
                        a = stack.peekptr();
                        Types::tSymbol->set(&t,ip->d.i);
                        a->t->getValue(a,&t,a);
                    }
                    ip++;
                    break;
                case OP_HASHSETSYMB:
                    {
                        Value t;
                        a = stack.popptr();
                        b = stack.popptr();
                        Types::tSymbol->set(&t,ip->d.i);
                        a->t->setValue(a,&t,b);
                    }
                    ip++;
                    break;
                case OP_LITERALSYMB:
                    Types::tSymbol->set(pushval(),ip->d.i);
                    ip++;
                    break;
                case OP_APPENDLIST:
                    a = popval(); // the value
                    
                    // if the value now on top of the stack is a list, 
                    // then we're appending to a list. Otherwise, the value UNDER THAT
                    // must be a hash, and that top value must be the key.
                    // Of course, this will cause problems if lists become hashable,
                    // and therefore able to become keys.
                    b = stack.peekptr(0);
                    if(b->t != Types::tList){
                        c = stack.peekptr(1);
                        if(c->t == Types::tHash){
                            Types::tHash->get(c)->set(b,a);
                            stack.popptr(); // discard the key
                        } else 
                            throw RUNT(EX_NOTCOLL,"attempt to set value in non-hash or list");
                    } else {
                        b = Types::tList->get(b)->append();
                        b->copy(a);
                    }
                    ip++;
                    break;
                case OP_DEF:{
                    WriteLock lock=WL(&ang->names);
                    const StringBuffer& sb = popString();
                    if(ang->names.isConst(sb.get(),false))
                        throw AlreadyDefinedException(sb.get());
                    int idx = ip->d.i ? ang->names.addConst(sb.get()):ang->names.add(sb.get());
                    ang->names.getVal(idx)->copy(popval());
                    ip++;
                    break;
                }
                case OP_CONSTEXPR:
                    pushval()->copy(ip->d.constexprval);
                    ip++;
                    break;
                case OP_COMPILEIF:
                    if(!popBool()){
                        if(ang->tokeniserTrace)printf("SKIPPING STARTS\n");
                        ang->isSkipping = true;
                    }
                    ip++;
                    break;
                case OP_TRY:
                    // make us ready to catch a throw
                    catchstack.peekptr()->push(ip->d.catches);
                    ip++;
                    break;
                case OP_ENDTRY:
                    // and pop the catches
                    catchstack.peekptr()->pop();
                    ip++;
                    break;
                case OP_THROW:
                    a = popval();
                    if(a->t != Types::tSymbol)
                        throw RUNT(EX_BADTHROW,"throw should throw a symbol");
                    b = popval();
                    
                    if(!throwAngortException(a->v.i,b)){
                        ReadLock lock(Types::tSymbol);
                        // we couldn't find an Angort handler - print msg and reset IP
                        const StringBuffer &sbuf = b->toString();
                        printf("unhandled throw instruction: %s (%s)\n",
                               Types::tSymbol->getString(a->v.i),sbuf.get());
                        if(ip && ang->debuggerHook)(*ang->debuggerHook)(this);
                        ip=NULL;
                        throw RUNT(EX_UNHANDLED,"").set("Angort exception: %s (%s)\n",
                                                        Types::tSymbol->getString(a->v.i),sbuf.get());
                    }
                    break;
                default:
                    throw RUNT(EX_BADOP,"unknown opcode");
                }
            } catch(Exception e){
                Value vvv;
                Types::tString->set(&vvv,e.what());
                
                if(!throwAngortException(e.id,&vvv)){
                    printf("Angort exception: %s\n",e.what());
                    // avoids debugger running with null IP when
                    // run() recurses, which it can because of
                    // runValue(). The flow here would be:
                    // - exception thrown
                    // - debugger entered
                    // - IP reset
                    // - rethrow
                    // - caught in next level of run() up
                    // - debugger re-entered with null ip
                    
                    if(ip&&ang->debuggerHook)(*ang->debuggerHook)(this);
                    // set IP and runtime
                    e.ip = ip;
                    e.run = this;
                    // clear IP here
                    ip=NULL;
                    // and rethrow.
                    throw e;
                }
            }
        }
    } catch(Exception e){
        // this is called when the outer handler throws
        // an error, i.e. the error was not handled by an
        // Angort try-catch block.
        // set IP and runtime
        e.ip = ip;
        e.run = this;
        // clear the catchstack, but push another empty entry on there
        catchstack.clear();
        catchstack.pushptr()->clear();
        // destroy any iterators left lying around
        while(!loopIterStack.isempty()){
            loopIterStack.popptr()->clr();
        }
        loopIterCt=0;
        // and the locals stack too
        locals.clear();
        // before we delete the rstack, print it
        printAndDeleteStoredTrace();
        rstack.clear();
        throw e; // and rethrow upstairs.
    }
    
leaverun:
    ip=NULL;
    catchstack.pop();
}

void Angort::startDefine(const char *name){
#if DEBCLOSURES
    printf("---Now defining %s\n",name);
#endif
    int idx;
    WriteLock lock=WL(&names);
    if(isDefining())
        throw SyntaxException("cannot define a word inside another");
    if((idx = names.get(name,false))<0)
        idx = names.add(name); // words are NOT constant; they can be redefined.
    else
        if(names.getEnt(idx)->isConst)
            throw SyntaxException("").set("cannot redefine constant '%s'",name);
    wordValIdx = idx;
}


void Angort::endDefine(CompileContext *c){
    WriteLock lock=WL(&names);
    if(!isDefining())
        throw SyntaxException("not defining a word");
    // make sure we have no dangling constructs
    c->checkStacksAtEnd();
    // get the codeblock out of the context and set it up.
    CodeBlock *cb = c->cb;
    cb->setFromContext(c);
#if DEBCLOSURES
    printf("End of define.\n");
    c->dump(); //snark
    cb->dump();
#endif
    
    Value *wordVal = names.getVal(wordValIdx);
    
    Types::tCode->set(wordVal,cb);
    names.setSpec(wordValIdx,c->spec);
    wordValIdx = -1;
}

void Angort::compileParamsAndLocals(){
    
    if(!isDefining() && !inSubContext())
        throw SyntaxException("cannot use |..| outside a word definition or code literal.");
    if(context->getCodeSize()!=0)
        throw SyntaxException("|..| must come first in a word definition or code literal");
    
    int paramct=0; // number of params to pop
    // we start parsing params, then switch
    // to locals - the only difference is that
    // params are popped off the stack
    bool parsingParams = true;
    
    char namebuf[128];
    
    for(;;){
        int t = tok.getnext();
        Type *typ;
        
        switch(t){
        case T_IDENT:
            // add a new local token; and
            // add to the pop count if it's a parameter
            strncpy(namebuf,tok.getstring(),128); // have to copy out; it gets overwritten by next tok
            if(tok.getnext()==T_DIV) { // parameters can be x/type,foo/type..
                if(tok.getnext()!=T_IDENT)
                    throw SyntaxException("expected a type in parameter list after /");
                if(!parsingParams)
                    throw SyntaxException("types only supported on parameters");
                
                typ = Type::getByName(tok.getstring());
                if(!typ)
                    throw SyntaxException("").set("unknown type in parameter list: %s",tok.getstring());
            } else {
                tok.rewind();
                typ=NULL;
            }
            
            context->addLocalToken(namebuf,typ);
            if(parsingParams)
                paramct++;
            
            break;
        case T_COLON:
            // switch to parsing local variables only
            parsingParams = false;
            break;
        case T_COMMA:
            // skip - it's a delimiter
            break;
        case T_PIPE:
            /// and finish, setting the number of params in the context
            context->setParamCount(paramct);
            return;
        case T_END:
            throw SyntaxException("parameter/local specs must be on a single line");
        }
    }
}

bool Angort::fileFeed(const char *name,bool rethrow){
    const char *oldName = tok.getname();
    int oldLN = lineNumber;
    lineNumber=1;
#if defined(SOURCEDATA)
    // we duplicate the filename so that we can always access it
    const char *fileName = strdup(name); 
    tok.setname(fileName);
#endif
    FILE *ff = fopen(name,"r");
    try{
        char buf[1024];
        if(!ff)
            throw Exception(EX_NOTFOUND).set("cannot open %s",name);
        while(fgets(buf,1024,ff)!=NULL){
            feed(buf);
        }
        tok.setname(oldName);
        lineNumber=oldLN;
        fclose(ff);
    }catch(Exception e){
        if(rethrow) throw e;
        printf("Error in file %s: %s\n",tok.getname(),e.what());
        printf("Last line: %s\n",getLastLine());
        tok.setname(oldName);
        lineNumber=oldLN;
        return false;
    } 
    return true;
}

void Angort::compile(const char *s,Value *out){
    //    RUNT(EX_NOTSUP,"no longer supported");
    // trick the system into thinking we're defining a colon
    // word for one line only
    
    pushCompileContext();
    feed(s);
    compile(OP_END); // make sure it's terminated
    CompileContext *c = popCompileContext();
    c->cb->setFromContext(c);
    
    Types::tCode->set(out,c->cb);
    
}

void CompileContext::reset(CompileContext *p,Tokeniser *tok){
    parent = p;
    compileCt=0;
    // only delete the codeblock if it didn't end up
    // being used, possibly due to a syntax error.
    if(cb && !cb->used)delete cb;
    // create the new codeblock we're going to compile into
    cb = new CodeBlock();
    //        compileBuf[0].opcode=1; //OP_END
    paramCt=0;
    localTokenCt=0;
    closureCt=0;
    localsClosed=0;
    tokeniser=tok;
    freeClosureList();
    if(spec){
        free((void *)spec);
        spec=NULL;
    }
    leaveListHead = -1;
    cstack.clear();
}

ClosureTableEnt *CompileContext::makeClosureTable(int *count){
    *count = closureListCt;
    if(!closureList)return NULL; // make a null table if there aren't any
    
    ClosureTableEnt *table = new ClosureTableEnt[closureListCt];
    ClosureTableEnt *t = table;
    
    // go through the list of closure entries resolving each
    // list entry to make an entry in the table - these will
    // be tuples of (levelup,idx) where levelup is how many levels
    // up the closure stack we will find the variable, and idx is
    // where in that closure's block it will be. There may also
    // be dummy entries in the list, which will have a null codeblock;
    // these will have a (-1,-1) table entry.
    
    for(ClosureListEnt *p=closureList;p;p=p->next){
        if(p->c == NULL){
            // dummy entry
            t->levelsUp = -1;
            t->idx = -1;
        } else {
            // work out how far up the stack the entry is
            int level=0;
            CompileContext *cc;
            for(cc=this;cc;cc=cc->parent){
                if(cc->cb==p->c)break;
                level++;
            }
            if(!cc)throw WTF; // didn't find it, and it should be there!
            t->levelsUp = level;
            t->idx = p->i;
            cdprintf("Closure table : Setting entry %d to level %d, index %d",(int)(t-table),level,t->idx);
        }
        t++;
    }
    return table;
}

void CompileContext::closeAllLocals(){
    cdprintf("CLOSING ALL LOCALS");
    for(int i=0;i<localTokenCt;i++){
        convertToClosure(localTokens[i]);
    }
}

void CompileContext::convertOp(int oldlocalidx,int newlocalidx,int fromcode,int tocode){
    char buf[1024];
    Instruction *inst=compileBuf;
    for(int i=0;i<compileCt;i++,inst++){
        cdprintf("   %s  %d",inst->getDetails(buf,1024),inst->d.i);
        if(inst->opcode == fromcode && inst->d.i == oldlocalidx){
            cdprintf("Rehashing to %d",newlocalidx);
            inst->opcode = tocode;
            inst->d.i = newlocalidx;
        }
    }
}


void CompileContext::convertToClosure(const char *name){
    cdprintf("Convert to closure for cb %p, %s",cb,name);
    // find which local this is
    int previdx;
    for(previdx=0;previdx<localTokenCt;previdx++)
        if(!strcmp(localTokens[previdx],name))break;
    cdprintf("Previous index (i.e. local index) of this variable is %d",previdx);
    
    if(previdx==localTokenCt)throw WTF; // didn't find it.
    // got it. Now set this as a closure.
    if((1<<previdx) & localsClosed){
        cdprintf("%s is already closed",name);
        return; // it's already converted.
    }
    
    
    localsClosed |= 1<<previdx; // set it to be closed
    
    int localIndex = localIndices[previdx];
    cdprintf("Local index is currently %d",localIndex);
    cdprintf("Closure list count is %d",closureListCt);
    
    /*  old code, see comment below for an explanation
       
       localIndices[previdx] = closureCt++;
       // add an entry to the local closure table
       addClosureListEnt(cb,localIndices[previdx]);
     */
    
    // complication here due to the way we a codeblock can
    // sometimes close functions itself, rather than have functions
    // closed in it from another codeblock. This happens when closeAllLocals()
    // runs because "yield" has been encountered.
    
    // create a new entry in the closure table for this codeblock,
    // whose index is the current closure.
    cdprintf("addClosureListEnt 1");
    addClosureListEnt(cb,closureCt++);
    // but there may be closures here already, so set the local
    // index for the closures to the most recent entry in that table.
    cdprintf("local index %d set to %d",previdx,closureListCt-1);
    int newLocalIndex = localIndices[previdx] = closureListCt-1;
    
    // convert all access of the local into the closure
    Instruction *inst = compileBuf;
    cdprintf("Beginning scan to convert local %d into closure %d",
             localIndex,newLocalIndex);
    
    convertOp(localIndex,newLocalIndex,OP_LOCALGET,OP_CLOSUREGET);
    convertOp(localIndex,newLocalIndex,OP_LOCALSET,OP_CLOSURESET);
    convertOp(localIndex,newLocalIndex,OP_LOCALINC,OP_CLOSUREINC);
    convertOp(localIndex,newLocalIndex,OP_LOCALDEC,OP_CLOSUREDEC);
    cdprintf("Ending scan");
    
    // now decrement all indices of locals greater than this.
    // Firstly do this in the table.
    
    cdprintf("Now decrementing subsequent tokens");
    for(int i=0;i<localTokenCt;i++){
        cdprintf("Token %d : %s",i,localTokens[i]);
        if(!isClosed(i) && localIndices[i]>localIndex){
            localIndices[i]--;
            cdprintf("  decremented to %d",localIndices[i]);
        }
    }
    
    // Then do it in the code generated thus far.
    
    inst = compileBuf;
    cdprintf("Now decrementing local accesses in generated code");
    for(int i=0;i<compileCt;i++,inst++){
        char buf[1024];
        cdprintf("scanning instruction   %s  %d",inst->getDetails(buf,1024),inst->d.i);
        if((
            inst->opcode == OP_LOCALGET || 
            inst->opcode == OP_LOCALINC || 
            inst->opcode == OP_LOCALDEC || 
            inst->opcode == OP_LOCALSET
            ) &&
           inst->d.i > localIndex){
            inst->d.i--;
            cdprintf("  decremented to %d",inst->d.i);
        }
    }
    
    cdprintf("Exiting convertToClosure()");
}

int CompileContext::findOrCreateClosure(const char *name){
    // first, scan all functions above this for a local variable.
    int localIndexInParent=-1;
    // this is the context which actually contains the data within its block.
    CompileContext *parentContainingVariable;
    
    cdprintf("---------------- findOrCreateClosure: Looking for %s",name);
    for(parentContainingVariable=parent;parentContainingVariable;
        parentContainingVariable=parentContainingVariable->parent){
        cdprintf("   Looking in %p",parentContainingVariable);
        if((localIndexInParent = parentContainingVariable->getLocalToken(name))>=0){
            cdprintf("FOUND AT LOCAL INDEX %d",localIndexInParent);
            // got it. If not already, turn it into a closure (which will add it to
            // the closure table of that function)
            if(!parentContainingVariable->isClosed(localIndexInParent)){
                cdprintf("     Got it, not closed, closing.");
                parentContainingVariable->convertToClosure(name);
            } else {
                cdprintf("     Got it and already closed.");
            }
            
            break;
        }
    }
    if(localIndexInParent<0)
    {
        cdprintf("returning, not found");
        return -1; // didn't find it
    }
    cdprintf("Local index in parent is %d",localIndexInParent);
    
    // get the index within the closure block.
    
    // CRASH 120817 
    // This appears to be where it's going wrong - this is actually the local index
    // in the closure TABLE, not the block. We probably need to dereference again.
    localIndexInParent = parentContainingVariable->getLocalIndex(localIndexInParent);
    // localIndexInParent is now the index in the containing *table*, not the *block*
    // (which is what we need).
    // so let's try to dereference from the closure table into the block in that context.
    cdprintf("Local index in table of parent is %d",localIndexInParent);
    ClosureListEnt *e = parentContainingVariable->getClosureListEntByIdx(localIndexInParent);
    localIndexInParent = e->i;
    
#if DEBCLOSURES
    parentContainingVariable->dumpClosureList();
#endif    
    
    cdprintf("Local index (in context %p) in closure block is %d",
             parentContainingVariable,localIndexInParent);
    
    
    // scan the closure table to see if we have this one already
    int nn=0;
    for(ClosureListEnt *p=closureList;p;p=p->next,nn++){
        cdprintf("Scanning table for %p/%d - got %p/%d",
                 parentContainingVariable->cb,localIndexInParent,p->c,p->i);
        if(p->c == parentContainingVariable->cb && p->i == localIndexInParent)
            return nn;
    }
    // if not, add it.
    cdprintf("addClosureListEnt 2");
    return addClosureListEnt(parentContainingVariable->cb,localIndexInParent);
}



void Angort::endPackageInScript(){
    WriteLock lock=WL(&names);
    // see the similar code below in include().
    // pop the namespace stack
    int idx=names.pop();
    Types::tNSID->set(run->pushval(),idx);
    names.setPrivate(false); // and clear the private flag
}

void Angort::include(const char *filename,bool isreq,bool mightNotExist){
    // find the file
    
    const char *fh = findFile(filename);
    if(!fh){
        if(!mightNotExist)
            throw FileNotFoundException(filename);
        else return;
    }
    
    int oldDir = open(".",O_RDONLY); // get the FD for the current directory so we can go back
    
    // first, find the real path of the file
    char *path=realpath(fh,NULL);
    free((void *)fh); // and free the buffer
    
    if(!path)
        throw Exception(EX_NOTFOUND).set("cannot open file : %s",fh);
    
    // now get the directory separator
    char *file = strrchr(path,'/');
    *file++ = 0; // and the file name
    
    // change to that directory, so all future reads are relative to there
    if(chdir(path))
        throw RUNT(EX_BADINC,"unable to switch directory in 'include'");
    
    TokeniserContext c;
    tok.saveContext(&c);
    fileFeed(file);
    tok.restoreContext(&c);
    
    if(isreq){
        // pop the namespace stack
        int idx=names.pop();
        // push the idx of the package which was defined. 
        // A bit dodgy since this isn't taking place in
        // a code block..
        Types::tNSID->set(run->pushval(),idx);
    }
    
    names.setPrivate(false); // and clear the private flag
    
    
    free(path);
    if(fchdir(oldDir))
        throw RUNT(EX_BADINC,"unable to reset directory in 'include'");
    close(oldDir);
}

void Angort::constCheck(int name){
    if(names.getEnt(name)->isConst)
        throw RUNT(EX_SETCONST,"").set("attempt to set constant %s",tok.getstring());
}

void Angort::parseVarAccess(int token){
    WriteLock lock=WL(&names);
    
    int t,opcode;
    if(tok.getnext()!=T_IDENT)
        throw SyntaxException(NULL).set("expected identifier after %s",
                                        tok.prev().s);
    if((t = context->getLocalToken(tok.getstring()))>=0){
        // it's a local variable
        switch(token){
        case T_QUESTION:
            opcode = context->isClosed(t) ? OP_CLOSUREGET : OP_LOCALGET;
            break;
        case T_PLING:
            opcode = context->isClosed(t) ? OP_CLOSURESET : OP_LOCALSET;
            break;
        case T_VARINC:
            opcode = context->isClosed(t) ? OP_CLOSUREINC : OP_LOCALINC;
            break;
        case T_VARDEC:
            opcode = context->isClosed(t) ? OP_CLOSUREDEC : OP_LOCALDEC;
            break;
        default:
            throw WTF;
        }
        compile(opcode)->d.i = context->getLocalIndex(t);
    } else if((t=context->findOrCreateClosure(tok.getstring()))>=0){
        // it's a variable defined in a function above
        switch(token){
        case T_QUESTION: opcode = OP_CLOSUREGET; break;
        case T_PLING: opcode = OP_CLOSURESET; break;
        case T_VARINC: opcode = OP_CLOSUREINC;break;
        case T_VARDEC: opcode = OP_CLOSUREDEC;break;
        default: throw WTF;
        }
        compile(opcode)->d.i = t;
    } else if((t = names.get(tok.getstring()))>=0){
        // it's a global
        Value *v = names.getVal(t);
        if(v->t == Types::tProp){
            switch(token){
            case T_QUESTION: opcode = OP_PROPGET;break;
            case T_PLING: opcode = OP_PROPSET;break;
            case T_VARINC:
            case T_VARDEC:
                throw SyntaxException("cannot increment/decrement a property");
            default:throw WTF;
            }
            // it's a property
            compile(opcode)->d.prop = v->v.property;
        } else {
            // it's a global; use it
            switch(token){
            case T_QUESTION:opcode = OP_GLOBALGET;break;
            case T_PLING:constCheck(t);opcode = OP_GLOBALSET;break;
            case T_VARINC:constCheck(t);opcode = OP_GLOBALINC;break;
            case T_VARDEC:constCheck(t);opcode = OP_GLOBALDEC;break;
            default:throw WTF;
            }
            compile(opcode)->d.i = t;
        }
    } else if(isupper(*tok.getstring())){
        // if it's upper case, immediately define as a global
        switch(token){
        case T_QUESTION:opcode = OP_GLOBALGET;break;
        case T_PLING:opcode = OP_GLOBALSET;break;
        case T_VARINC:
        case T_VARDEC:
            throw SyntaxException(NULL).set("cannot increment/decrement unset global %s",tok.getstring());
        default:throw WTF;
        }
        compile(opcode)->d.i=
              findOrCreateGlobal(tok.getstring());
    } else {
        throw SyntaxException(NULL)
              .set("unknown variable: %s",tok.getstring());
    }
}


void Angort::feed(const char *buf){
    // clear exception data in default thread only
    run->resetStop();
    
    //    printf("FEED STARTING: %s\n",buf);
    
    if(printLines)
        printf("%d >>> %s\n",lineNumber,buf);
    
    if(!isDefining() && context && !inSubContext()) // make sure we're reset unless we're compiling or subcontexting
        context->reset(NULL,&tok);
    
    strcpy(lastLine,buf);
    tok.settrace(tokeniserTrace);
    tok.reset(buf);
    // the tokeniser will reset its idea of the line number,
    // because we reset it at the start of all input.
    tok.setline(lineNumber);
    
    if(hereDocEndString!=NULL){
        if(!strcmp(buf,hereDocEndString)){
            hereDocEndString = NULL;
            
            // remove the trailing NL from the heredoc string
            if(strlen(hereDocString)>0){ // shouldn't happen
                char *qq = hereDocString+(strlen(hereDocString)-1);
                if(*qq == '\n')*qq=0;
            }
            
            // compile and if necessary run the code to stack the string
            compile(OP_LITERALSTRING)->d.s = strdup(hereDocString);
            free(hereDocString);
            hereDocString=NULL;
            if(!isDefining() && !inSubContext()){
                compile(OP_END);
                // run in default runtime
                run->run(context->getCode());
                clearAtEndOfFeed();
            }
        }
        else {
            if(!hereDocString){
                hereDocString = strdup(buf);
            } else {
                int len = strlen(hereDocString)+strlen(buf)+1;
                hereDocString = (char *)realloc(hereDocString,len);
                strcpy(hereDocString+strlen(hereDocString),buf);
            }
        }
        lineNumber++;
        return;
    }
    
    // heredocs start the line with -- 
    if(buf[0] == '-' && buf[1] == '-'){
        hereDocEndString=strdup(buf);
        return;
    }
    
    if(isSkipping){
        if(tokeniserTrace)printf("SKIPPING %s\n",buf);
        bool isend = !strncmp(buf,"endcompileif",12);
        if(isend || !strncmp(buf,"elsecompileif",12)){
            if(tokeniserTrace)printf("SKIPPING OFF %s\n",buf);
            isSkipping=false;
            if(isend)inCompileIf=false;
        }
        return;
    }
    
    
    int here; // instruction index variable used in different ways
    try {
        for(;;){
            
            int t = tok.getnext();
            switch(t){
            case T_INCLUDEIFEXISTS:
            case T_INCLUDE:{
                char buf[1024];
                // will recurse
                if(!tok.getnextstring(buf))
                    throw SyntaxException("expected a filename after 'include'");
                //                if(tok.getnext()!=T_END)
                //                    throw SyntaxException("include must be at end of line");
                
                include(buf,false,t==T_INCLUDEIFEXISTS);
                break;
            }
            case T_REQUIRE:{
                // like include, but a package should
                // be created whose namespace idx will be on the stack,
                // ready for import or list-import.
                char buf[1024];
                // will recurse
                if(!tok.getnextstring(buf))
                    throw FileNameExpectedException();
                //                if(tok.getnext()!=T_END)
                //                    throw SyntaxException("require must be at end of line");
                
                // is there a package name specifier (so we can precheck
                // it's not there already?)
                char *fnstart = strchr(buf,':');
                if(fnstart){
                    // yes - do that.
                    *fnstart=0;
                    if(loadedLibraries.find(buf)){
                        // it's there. Just return the NSID
                        ReadLock rlnames(&names);
                        Namespace *sp = names.getSpaceByName(buf);
                        Types::tNSID->set(run->pushval(),sp->idx);
                    } else {
                        // no, do the include
                        include(fnstart+1,true);
                    }
                } else {
                    // no ':', so there's no package name.
                    include(buf,true);
                }
                break;
            }
            case T_COMPILEIF:
                if(isDefining())
                    throw SyntaxException("'compileif' not allowed in a definition");
                compile(OP_COMPILEIF);
                inCompileIf=true;
                break;
            case T_ENDCOMPILEIF:
                if(isDefining())
                    throw SyntaxException("'compileendif' not allowed in a definition");
                inCompileIf=false;
                break;
            case T_ELSECOMPILEIF:
                if(isDefining())
                    throw SyntaxException("'elsecompileif' not allowed in a definition");
                if(!inCompileIf)
                    throw SyntaxException("'elsecompileif' without 'compileif'");
                isSkipping=true;
                break;
            case T_PRIVATE:
                {
                    WriteLock lock=WL(&names);
                    names.setPrivate(true);
                }
                break;
            case T_PUBLIC:
                {
                    WriteLock lock=WL(&names);
                    names.setPrivate(false);
                }
                break;
            case T_PACKAGE:
                {
                    WriteLock lock=WL(&names);
                    // start a new package.
                    char buf[256];
                    if(!tok.getnextident(buf))
                        throw SyntaxException("expected a package name");
                    int idx = names.create(buf);
                    // stack it, we're now defining things in
                    // this package and will be until fileFeed() returns
                    // in include()
                    names.push(idx);
                }
                break;
            case T_BACKTICK:{
                char buf[256];
                if(!tok.getnextidentorkeyword(buf))
                    throw SyntaxException("expected a symbol after backtick");
                compile(OP_LITERALSYMB)->d.i=Types::tSymbol->getSymbol(buf);
                break;
            }
            case T_CONST: // const syntax = <val> const <ident>
                {
                    if(isDefining())
                        throw SyntaxException("'const' not allowed in a definition");
                    if(tok.getnext()!=T_IDENT)
                        throw SyntaxException("expected an identifier");
                    
                    if(names.isConst(tok.getstring(),false))
                        throw AlreadyDefinedException(tok.getstring());
                    
                    int n = names.addConst(tok.getstring());
                    // we write an instruction to 
                    // store this const
                    compile(OP_GLOBALSET)->d.i=n;
                }
                break;
            case T_DEF:
                compile(OP_DEF)->d.i = 0;
                break;
            case T_DEFCONST:
                compile(OP_DEF)->d.i = 1;
                break;
            case T_RECURSE:
                compile(OP_RECURSE);
                break;
            case T_SELF:
                compile(OP_SELF);
                break;
            case T_COLON:
                if(isDefining()){
                    // the only valid use of ":" in a definition is in a specstring.
                    char spec[1024];
                    if(!tok.getnextstring(spec))
                        throw SyntaxException("").set("expected spec string after second ':' in definition, got '%s'",tok.getstring());
                    context->setSpec(spec);
                } else {            
                    char defname[256];
                    if(!tok.getnextident(defname))
                        throw SyntaxException("").set("expected a word name, not %s (toktype %d)",tok.getstring(),tok.getcurrent());
                    startDefine(defname);
                }
                break;
            case T_DOT:
                compile(OP_DOT);
                break;
            case T_SEMI:
                if(!isDefining())
                    throw SyntaxException("; not allowed outside a definition");
                compile(OP_END);
                endDefine(context);
#if DEBCLOSURES
                printf("defined - %d ops\n",context->getCodeSize());
#endif
                context->reset(NULL,&tok);
                break;
                
            case T_CASES:
                context->pushmarker();
                break;
            case T_CASE:
                t = context->pop();
                // write jump of here+1 to location t
                context->resolveJumpForwards(t,1);
                t = context->pop();
                // write t to current location with dummy opcode,
                // and stack that.
                context->pushhere();
                compile(OP_DUMMYCASE)->d.i = t;
                break;
            case T_OTHERWISE:
                t = context->pop();
                while(t>=0){
                    Instruction *i = context->getInst(t);
                    if(i->opcode!=OP_DUMMYCASE)
                        throw SyntaxException("error in cases");
                    int nextaddr = i->d.i;
                    i->opcode=OP_JUMP;
                    context->resolveJumpForwards(t);
                    t = nextaddr;
                }
                break;
                
            case T_IF:
                // at compile time, push this compiler location onto the compiler
                // stack, and then output a dummy instruction
                context->pushhere();
                compile(OP_IF);
                break;
            case T_ELSE:
                // pop off the old reference
                t = context->pop();
                // resolve it to the instruction past this one
                context->resolveJumpForwards(t,1);
                // push the jump instruction we're about to compile
                context->pushhere();
                compile(OP_JUMP);
                break;
            case T_THEN:
                // resolve the unresolved jump
                // pop off the old reference
                t = context->pop();
                // resolve it to the next instruction
                context->resolveJumpForwards(t);
                // no need for an actual opcode
                break;
            case T_EACH:
                if(tok.getnext()!=T_OCURLY)
                    throw SyntaxException("each must be followed by {");
                compile(OP_ITERSTART); // creates and starts the iterator
                context->pushhere(); // loop point
                context->pushleave(); // and which loop we're in
                context->compileAndAddToLeaveList(OP_ITERLEAVEIFDONE);
                break;
            case T_OCURLY://start loop
                compile(OP_LOOPSTART);
                // stack the location
                context->pushhere();
                // and which loop we're in
                context->pushleave();
                break;
            case T_CCURLY:{
                // get the leave list in the current context
                Instruction *leaveList = context->leaveListFirst();
                context->clearLeaveList();
                // pop the context, getting the loop start. We still have a pointer
                // to the previous context's leaveList.
                context->popleave();
                t = context->pop();
                here = context->getCodeSize();
                // compile a jump and set the destination to the start of the loop
                Instruction *loc=compile(OP_JUMP);
                loc->d.i = (t-here);
                loc++; // get instruction after loop
                // now go through all the opcodes in the previous context's leave list and resolve them
                while(leaveList){
                    // do it this way so we don't overwrite the next pointer before
                    // we read it!
                    Instruction *next = context->leaveListNext(leaveList);
                    leaveList->d.i = (loc-leaveList);
                    leaveList = next;
                }
                break;
            }
            case T_INT:
                compile(OP_LITERALINT)->d.i = tok.getint();
                break;
            case T_LONG:
                compile(OP_LITLONG)->d.l = tok.getlong();
                break;
            case T_FLOAT:
                compile(OP_LITERALFLOAT)->d.f = tok.getfloat();
                break;
            case T_DOUBLE:
                compile(OP_LITDOUBLE)->d.df = tok.getdouble();
                break;
            case T_LEAVE:
                context->compileAndAddToLeaveList(OP_LEAVE);
                break;
            case T_IFLEAVE:context->compileAndAddToLeaveList(OP_IFLEAVE);break;
            case T_MUL:compile(OP_MUL);break;
            case T_DIV:compile(OP_DIV);break;
            case T_SUB:compile(OP_SUB);break;
            case T_ADD:compile(OP_ADD);break;
            case T_CMP:compile(OP_CMP);break;
            case T_PERC:compile(OP_MOD);break;
            case T_DUP:compile(OP_DUP);break;
            case T_OVER:compile(OP_OVER);break;
            case T_SWAP:compile(OP_SWAP);break;
            case T_DROP:compile(OP_DROP);break;
            case T_AT:
            case T_CALL:compile(OP_CALL);break;
            case T_EQUALS:compile(OP_EQUALS);break;
            case T_NOT:compile(OP_NOT);break;
            case T_AND:compile(OP_AND);break;
            case T_OR:compile(OP_OR);break;
            case T_LT:compile(OP_LT);break;
            case T_GT:compile(OP_GT);break;
            case T_LE:compile(OP_LE);break;
            case T_GE:compile(OP_GE);break;
#if SOURCEDATA
            case T_BRK:compile(OP_NOP)->brk=true;break; // nop breakpoint
#else
            case T_BRK:compile(OP_NOP);break; // nop breakpoint
#endif
            case T_IDENT:
                {
                    WriteLock lock=WL(&names);
                    char *s = tok.getstring();
                    if((t = names.get(s))>=0){
                        // fast option for functions
                        Value *v = names.getVal(t);
                        if(v->t == Types::tNative)
                            compile(OP_FUNC)->d.func = v->v.native;
                        else if(v->t == Types::tProp)
                            throw SyntaxException(NULL)
                              .set("property '%s' requires ? or !",s);
                        else
                            compile(OP_GLOBALDO)->d.i = t;
                    } else if(barewords){
                        compile(OP_LITERALSYMB)->d.i=
                              Types::tSymbol->getSymbol(s);
                    } else {
                        throw SyntaxException(NULL)
                              .set("unknown identifier: %s",s);
                    }
                    break;
                }
            case T_STRING:
                {
                    char *s = strdup(tok.getstring());
                    compile(OP_LITERALSTRING)->d.s = s;
                    break;
                }
            case T_HASHGETSYMB:{
                char buf[256];
                if(!tok.getnextident(buf))
                    throw SyntaxException("expected a symbol after backtick");
                compile(OP_HASHGETSYMB)->d.i=Types::tSymbol->getSymbol(buf);
                break;
                
            }
            case T_INC:compile(OP_INC)->d.i=1;break;
            case T_DEC:compile(OP_INC)->d.i=-1;break;
            case T_PLING: 
            case T_QUESTION:
            case T_VARINC:
            case T_VARDEC:
                parseVarAccess(t);
                break;
            case T_NOTEQUAL:
                compile(OP_NEQUALS);
                break;
            case T_HASHSETSYMB:{
                char buf[256];
                if(!tok.getnextident(buf))
                    throw SyntaxException("expected a symbol after backtick");
                compile(OP_HASHSETSYMB)->d.i=Types::tSymbol->getSymbol(buf);
                break;
            }
            case T_GLOBAL:{
                WriteLock lock=WL(&names);
                if(tok.getnext()!=T_IDENT)
                    throw SyntaxException(NULL)
                      .set("expected an identifier, got %s",tok.getstring());
                
                // get superindex of name if it exists
                // "false" here so we don't look in imported namespaces!
                int superindex = names.get(tok.getstring(),false);
                
                // if the name exists, that's fine if it's not constant.
                if(superindex>=0){
                    if(names.getEnt(superindex)->isConst){
                        throw AlreadyDefinedException(tok.getstring());
                    }
                    // it's defined but not constant, ignore the
                    // global keyword so we don't redefine it.
                } else {
                    // name doesn't exists, make it.
                    names.add(tok.getstring());
                }
                break;
            }
            case T_DOUBLEANGLEOPEN:
            case T_OPREN:// open lambda
#if DEBCLOSURES
                printf("---Pushing: current context is %p[cb:%p], ",context,context->cb);
#endif
                pushCompileContext();
#if DEBCLOSURES
                printf("pushing into new lambda context  %p[cb:%p]\n",context,context->cb);
#endif
                break;
            case T_CPREN: // close lambda and stack a code literal
                {
                    if(!inSubContext())
                        throw SyntaxException("')' not inside a code literal");
                    
                    compile(OP_END); // finish the compile
                    CompileContext *lambdaContext = popCompileContext();
                    
                    // set the codeblock up
                    lambdaContext->cb->setFromContext(lambdaContext);
#if DEBCLOSURES
                    printf("End of lambda context.\n");
                    lambdaContext->dump();//snark
                    lambdaContext->cb->dump();
#endif
                    
                    // here, we compile LITERALCODE word with a codeblock created
                    // from the context.
                    
                    compile(OP_LITERALCODE)->d.cb = lambdaContext->cb;
                    lambdaContext->reset(NULL,&tok);
                }
                break;
            case T_DOUBLEANGLECLOSE:
                {
                    if(!inSubContext())
                        throw SyntaxException("')' not inside a code literal");
                    
                    compile(OP_END); // finish the compile
                    CompileContext *lambdaContext = popCompileContext();
                    
                    // set the codeblock up
                    lambdaContext->cb->setFromContext(lambdaContext);
                    //                    lambdaContext->dump();
                    
                    // here, we compile LITERALCODE word with a codeblock created
                    // from the context.
                    
                    // now we actually run that codeblock
                    Value *vv = new Value();
                    Types::tCode->set(vv,lambdaContext->cb);
                    run->runValue(vv);
                    
                    // we don't need the codeblock any more (note,
                    // if we ever GC codeblocks this will free twice)
                    delete lambdaContext->cb;
                    lambdaContext->reset(NULL,&tok);
                    // get the value that codeblock had
                    vv->copy(run->popval());
                    compile(OP_CONSTEXPR)->d.constexprval=vv;
                }
                break;
            case T_END:
                // just return if we're still defining
                // otherwise run the buffer we just made
                // in the default runtime!
                if(!isDefining() && !inSubContext()){
                    context->checkStacksAtEnd(); // check dangling constructs
                    compile(OP_END);
                    run->run(context->getCode());
                    clearAtEndOfFeed();
                }
                lineNumber++;
                return;
            case T_PIPE:
                compileParamsAndLocals();
                break;
            case T_OSQB: // create a new list or hash, depending on the next token
                if(tok.getnext()==T_PERC) {
                    compile(OP_NEWHASH);
                } else {
                    tok.rewind();
                    compile(OP_NEWLIST);
                }
                // if the next token is a close, just swallow it.
                if(tok.getnext()!=T_CSQB)
                    tok.rewind();
                break;
            case T_CSQB:
            case T_COMMA:
                compile(OP_APPENDLIST);
                break;
            case T_STOP:
                compile(OP_STOP);
                break;
            case T_SOURCELINE:
                compile(OP_LITERALINT)->d.i = tok.getline();
                break;
            case T_YIELD:
                context->closeAllLocals();
                compile(OP_YIELD);
                break;
            case T_TRY:
                {
                    context->pushhere();
                    compile(OP_TRY);
                    // start the catch set
                    ArrayList<Catch> *cp = context->catchSetStack.pushptr();
                    cp->clear();
                }
                break;
            case T_CATCH:
                {
                    if(context->catchSetStack.isempty())
                        throw SyntaxException("catch must be inside a try-endtry block");
                    ArrayList<Catch> *cp = context->catchSetStack.peekptr();
                    if(tok.getnext()!=T_COLON)
                        throw SyntaxException("Expected :symbol after catch");
                    if(tok.getnext()!=T_IDENT)
                        throw SyntaxException("Expected :symbol after catch");
                    
                    if(cp->count()==0){
                        // first catch - compile OP_JUMP and add to stack
                        context->pushhere();
                        compile(OP_JUMP);
                    } else {
                        // subsequent catches - compile OP_JUMP and set curcatch->end,
                        // to terminate the previous catch
                        context->curcatch->end = context->getCodeSize();
                        compile(OP_JUMP);
                    }
                    
                    // create new catch, set start to here
                    Catch *cat = cp->append();
                    cat->start = context->getCodeSize();
                    context->curcatch = cat;
                    
                    // add the symbols we're catching
                    for(;;){
                        int symbol = Types::tSymbol->getSymbol(tok.getstring());
                        *cat->symbols.append() = symbol;
                        int tk=tok.getnext();
                        if(tk==T_COMMA){
                            if(tok.getnext()!=T_IDENT)
                                throw SyntaxException("expected symbol after comma in catch");
                        } else {
                            tok.rewind();
                            break;
                        }
                    }
                }
                break;
            case T_CATCHALL:
                {
                    // see above for all this. Yes, duplicated.
                    if(context->catchSetStack.isempty())
                        throw SyntaxException("catch must be inside a try-endtry block");
                    ArrayList<Catch> *cp = context->catchSetStack.peekptr();
                    if(cp->count()==0){
                        // first catch - compile OP_JUMP and add to stack
                        context->pushhere();
                        compile(OP_JUMP);
                    } else {
                        // subsequent catches - compile OP_JUMP and set curcatch->end,
                        // to terminate the previous catch
                        context->curcatch->end = context->getCodeSize();
                        compile(OP_JUMP);
                    }
                    Catch *cat = cp->append();
                    cat->start = context->getCodeSize();
                    context->curcatch = cat;
                    *cat->symbols.append() = CATCHALLKEY; //ALL marker
                }
                break;
            case T_ENDTRY:
                {
                    if(context->catchSetStack.isempty())
                        throw SyntaxException("endtry must be terminate a try-endtry block");
                    ArrayList<Catch> *cpl = context->catchSetStack.peekptr();
                    if(!cpl->count())
                        throw SyntaxException("must be at least one catch in a try block");
                    
                    // terminate the final catch
                    context->curcatch->end = context->getCodeSize();
                    compile(OP_JUMP);
                    
                    // pop the location stored in the first catch
                    // (i.e. the OP_JUMP at the end of the actual
                    // try block) and make it jump here
                    int endloc = context->pop();
                    context->resolveJumpForwards(endloc);
                    
                    // now make ALL the catch ends jump JUST PAST here
                    ArrayListIterator<Catch> iter(cpl);
                    for(iter.first();!iter.isDone();iter.next()){
                        Catch *cat = iter.current();
                        context->resolveJumpForwards(cat->end,1);
                    }
                    
                    // pop the OP_TRY off the stack and create an
                    // exception hash for it
                    Instruction *tryIP = context->getInst(context->pop());
                    if(tryIP->opcode != OP_TRY)
                        throw SyntaxException("try/endtry mismatch");
                    
                    // create hash
                    IntKeyedHash<int> *exh = new IntKeyedHash<int>();
                    for(iter.first();!iter.isDone();iter.next()){
                        Catch *cat = iter.current();
                        ArrayListIterator<int> symiter(&cat->symbols);
                        for(symiter.first();!symiter.isDone();symiter.next()){
                            int *v = exh->set(*symiter.current());
                            *v = cat->start;
                        }
                    }
                    tryIP->d.catches = exh;
                    compile(OP_ENDTRY);
                    // this is a popptr - if it's pop(), bad things
                    // happen because we end up popping a copy
                    // of the array list (with the same data ptr!)
                    // which is immediately deleted.
                    context->catchSetStack.popptr();
                }
                break;
            case T_THROW:
                compile(OP_THROW);
                break;
            case T_DOUBLEQUERY:
                {
                    if(tok.getnext()!=T_IDENT)
                        throw SyntaxException(NULL)
                          .set("expected identifier after ?? - perhaps '%s' is a built in token?",
                               tok.getstring());
                    const char *s = getSpec(tok.getstring());
                    if(!s)s="no help found";
                    printf("%s: %s\n",tok.getstring(),s);
                }
                break;
            case T_WITH:
                if(tok.getnext()!=T_IDENT)
                    throw NamespaceExpectedException();
                names.pushWith(tok.getstring());
                break;
            case T_ENDWITH:
                names.popWith();
                break;
                
                
                
            default:
                throw SyntaxException(NULL).set("unhandled token: %s",
                                                tok.getstring());
            }
        }
        //        printf("------------ FEED ENDING: %s\n",buf);
    } catch(Exception e){
        clearAtEndOfFeed();
        throw e; // and rethrow
    }
    lineNumber++;
}


void Runtime::clearAtEOF(){
    while(!rstack.isempty()){
        rstack.popptr()->clear();
        catchstack.pop();
    }
    // destroy any iterators left lying around
    while(!loopIterStack.isempty()){
        loopIterStack.popptr()->clr();
    }
    catchstack.clear();
    
    loopIterCt=0;
    locals.clear();
    currClosure.clr();
}    


void Angort::clearAtEndOfFeed(){
    //    printf("Clearing at end of feed\n");
    // make sure we tidy up any state
    contextStack.clear(); // clear the context stack
    context = contextStack.pushptr();
    context->ang = this;
    context->reset(NULL,&tok); // reset the old context
    wordValIdx=-1;
    // make sure the return stack gets cleared otherwise
    // really strange things can happen on the next processed
    // line
    
    run->clearAtEOF();
}    

void Angort::disasm(const CodeBlock *cb, int indent){
    const Instruction *ip = cb->ip;
    const Instruction *base = ip;
    for(;;){
        int opcode = ip->opcode;
        run->showop(ip++,indent,base);
        if(opcode != OP_END || indent==0)
            printf("\n");
        if(opcode == OP_END)break;
    }
}

void Angort::disasm(const char *name){
    int idx = names.get(name);
    if(idx<0)
        throw RUNT(EX_NOTFOUND,"unknown function");
    Value *v = names.getVal(idx);
    if(v->t != Types::tCode)
        throw RUNT(EX_NOTFUNC,"not a function");
    
    disasm(v->v.cb);
}

const char *Instruction::getDetails(char *buf,int len) const{
#if SOURCEDATA
    snprintf(buf,len,"[%s] %s:%d/%d",opcodenames[opcode],
             file,line,pos);
#else
    snprintf(buf,len,"[%s]",opcodenames[opcode]);
#endif
    return buf;
}

const char *Angort::getSpec(const char *s){
    int idx = names.get(s);
    if(idx<0)
        return NULL;
    return names.getEnt(idx)->spec;
}

void Angort::list(){
    names.list();
}

void Runtime::dumpFrame(){
    printf("Frame data:\n");
    printf("  Curclosure: %s\n",currClosure.toString().get());
    printf("  Ret stack:\n");
    for(int i=0;i<rstack.ct;i++){
        Frame *f = rstack.peekptr(i);
        printf("   Rec:-%30s   Clos:%s\n",
               f->rec.toString().get(),
               f->clos.toString().get());
    }
    CycleDetector::getInstance()->dump();
}

void Angort::registerProperty(const char *name, Property *p, const char *ns,const char *spec){
    WriteLock lock=WL(&names);
    Namespace *sp = names.getSpaceByName(ns?ns:"std",true);
    int i = sp->addConst(name,false);
    sp->setSpec(i,spec);
    Value *v = sp->getVal(i);
    Types::tProp->set(v,p);
}


void Angort::registerBinop(const char *lhsName,const char *rhsName,
                           const char *opcode,BinopFunction f){
    bool r = false; // are we recursing?
    // first, deal with "supertypes": "str" registers for both
    // string and symbol, "number" registers for both int and float.
    if(!strcmp(lhsName,"str")){
        registerBinop("string",rhsName,opcode,f);
        registerBinop("symbol",rhsName,opcode,f);
        r=true;
    }
    if(!strcmp(rhsName,"str")){
        registerBinop(lhsName,"string",opcode,f);
        registerBinop(lhsName,"symbol",opcode,f);
        r=true;
    }
    if(!strcmp(lhsName,"number")){
        registerBinop("float",rhsName,opcode,f);
        registerBinop("integer",rhsName,opcode,f);
        r=true;
    }
    if(!strcmp(rhsName,"number")){
        registerBinop(lhsName,"float",opcode,f);
        registerBinop(lhsName,"integer",opcode,f);
        r=true;
    }
    if(r)return; // only do something if all types fully resolved
    
    // find types
    
    Type *lhs = Type::getByName(lhsName);
    if(!lhs)
        throw RUNT(EX_NOTFOUND,"").set("unknown type in binop def: %s",lhsName);
    Type *rhs = Type::getByName(rhsName);
    if(!rhs)
        throw RUNT(EX_NOTFOUND,"").set("unknown type in binop def: %s",rhsName);
    
    // find opcode and register
    
    for(int op=0;;op++){
        if(!opcodenames[op])
            throw RUNT(EX_BADOP,"").set("unknown opcode in binopdef: %s",opcode);
        if(!strcmp(opcodenames[op],opcode)){
            lhs->registerBinop(rhs,op,f);
            break;
        }
    }
}


int Angort::registerLibrary(LibraryDef *lib,bool import){
    Namespace *sp;
    {
        WriteLock lock=WL(&names);
        
        // make the namespace. Multiple imports into the same one
        // are permitted.
        sp = names.getSpaceByName(lib->name,true);
        
        // register the words
        for(int i=0;;i++){
            if(!lib->wordList[i].name)break;
            int id = sp->addConst(lib->wordList[i].name,false);
            Value *v = sp->getVal(id);
            Types::tNative->set(v,lib->wordList[i].f);
            sp->setSpec(id,lib->wordList[i].desc);
        }
        
        
        *libs->append() = lib;
    }
    
    if(lib->initfunc){
        (*lib->initfunc)(run,showinit);
    }
    
    // register the binops AFTER we init the function, so the
    // types are all sorted. Although that should be done by
    // static constructors.
    if(lib->binopList){
        for(int i=0;;i++){
            if(!lib->binopList[i].lhs)break;
            registerBinop(lib->binopList[i].lhs,
                          lib->binopList[i].rhs,
                          lib->binopList[i].opcode,
                          lib->binopList[i].f);
        }
    }
    if(import){
        WriteLock lock=WL(&names);
        names.import(sp->idx,NULL);
    }
    return sp->idx;
    
}

void Runtime::printTrace(){
    char buf[1024]; // buffer to write instruction details
    // first put the current frame on
    if(ip)
        ip->getDetails(buf,1024);
    else
        strcpy(buf,"unknown");
    printf("%s\n",buf);
    
    for(int i=0;i<rstack.ct;i++){
        Frame *p = rstack.peekptr(i);
        if(p->ip)
            p->ip->getDetails(buf,1024);
        else
            strcpy(buf,"unknown");
    }
    printf("%s\n",buf);
}


void Runtime::storeTrace(){
    char buf[1024]; // buffer to write instruction details
    char **ptr; // pointer to list item, itself a pointer
    
    if(storedTrace.count()){
        printf("Previous stored trace:\n");
        printAndDeleteStoredTrace();
    }
    // first put the current frame on
    if(ip)
        ip->getDetails(buf,1024);
    else
        strcpy(buf,"unknown");
    ptr = storedTrace.append();
    *ptr = strdup(buf);
    
    
    for(int i=0;i<rstack.ct;i++){
        Frame *p = rstack.peekptr(i);
        if(p->ip)
            p->ip->getDetails(buf,1024);
        else
            strcpy(buf,"unknown");
        ptr = storedTrace.append();
        *ptr = strdup(buf);
    }
}

void Runtime::printAndDeleteStoredTrace(){
    if(traceOnException){
        if(!storedTrace.count()){
            printf("No stored trace to print\n");
        } else {
            ArrayListIterator<char *> iter(&storedTrace);
            for(iter.first();!iter.isDone();iter.next()){
                printf("  from %s\n",*iter.current());
            }
        }
    }
    ArrayListIterator<char *> iter(&storedTrace);
    for(iter.first();!iter.isDone();iter.next()){
        free(*iter.current());
    }
    storedTrace.clear();
}

void Runtime::printStoredTrace(){
    if(!storedTrace.count()){
        printf("No stored trace to print (only works with exceptions)\n");
    } else {
        ArrayListIterator<char *> iter(&storedTrace);
        for(iter.first();!iter.isDone();iter.next()){
            printf("  from %s\n",*iter.current());
        }
    }
}


void Angort::resetAutoComplete(){
    ReadLock lock(&names);
    // build the new autocomplete list
    acList->clear();
    // first, add the tokens
    for(int i=0;tokens[i].word;i++){
        *acList->append()=strdup(tokens[i].word);
    }
    // and finally add the fully qualified name for all namespaces
    // (and unqualified name for imported namespaces)
    char buf[1024];
    for(int i=0;i<names.spaces.count();i++){
        Namespace *ns = names.getSpaceByIdx(i);
        const char *prefix = names.spaces.getName(i);
        for(int i=0;i<ns->count();i++){
            NamespaceEnt *ent = ns->getEnt(i);
            
            if(ent->isImported)
                *acList->append()=strdup(ns->getName(i));
            if(!ent->isPriv){
                strcpy(buf,prefix);
                strcat(buf,"$");
                strcat(buf,ns->getName(i));
                *acList->append()=strdup(buf);
            }
        }
    }
    acIndex=0;
}

const char *Angort::getNextAutoComplete(){
    for(;;){
        if(acIndex>=acList->count())return NULL;
        const char *s = *acList->get(acIndex);
        acIndex++;
        if(*s != '*')return s;
    }
}

const char *Angort::appendToSearchPath(const char *path){
    const char *prev = searchPath;
    int len = strlen(searchPath)+1+strlen(path);
    char *n = (char *)malloc(len+1);
    strcpy(n,searchPath);
    char *p = n+strlen(n);
    *p++=':';
    strcpy(p,path);
    searchPath = n;
    return prev;
}


/// comparator for ArrayList sorting of values with a comparator
/// object

// all very ugly, but I can't seem to avoid the static here, which sort
// of throws the templating out of the window (as far as I can see).

static ArrayListComparator<Value> *cmpObj;
int arrayCmp(const void *a,const void *b){
    const Value *va = (const Value *)a;
    const Value *vb = (const Value *)b;
    
    return cmpObj->compare(va,vb);
}

template<> void ArrayList<Value>::sort(ArrayListComparator<Value> *cmp){
    cmpObj=cmp;
    WriteLock lock=WL(this);
    qsort(data,ct,sizeof(Value),arrayCmp);
}

}
