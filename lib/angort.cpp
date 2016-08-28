/**
 * @file 
 *
 * 
 * @author $Author$
 * @date $Date$
 */


#define ANGORT_VERSION 262

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>

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
LIBNAME(stdmath),LIBNAME(stdenv);

namespace angort {

int Angort::getVersion(){
    return ANGORT_VERSION;
}

Angort *Angort::callingInstance=NULL;

Angort::Angort() {
    traceOnException=true;
    running = true;
    Types::createTypes();
    // create and set default namespace
    stdNamespace = names.create("std");
    // import everything from it
    names.import(stdNamespace,NULL);
    // and that's the namespace we're working in
    names.push(stdNamespace);
    lineNumber=1;
    
    rstack.setName("return");
    loopIterStack.setName("loop iterator");
    contextStack.setName("context");
    stack.setName("main");
    catchstack.setName("catchstack");
    
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
    registerLibrary(&LIBNAME(stdmath),true);
    registerLibrary(&LIBNAME(stdenv),true);
    
    
    
    // now the standard package has been imported, set up the
    // user package into which their words are defined.
    
    int userNamespace = names.create("user");
    names.import(userNamespace,NULL);
    names.push(userNamespace);
    
    debug=false;
    printLines=false;
    emergencyStop=false;
    assertDebug=false;
    assertNegated=false;
    wordValIdx=-1;
    barewords=false;
    autoCycleCount = autoCycleInterval = AUTOGCINTERVAL;
    loopIterCt=0;
    hereDocString = hereDocEndString = NULL;
    
    /// create the default, root compilation context
    context = contextStack.pushptr();
    context->reset(NULL,&tok);
    
    acList = new ArrayList<const char *>(128);
}

Angort::~Angort(){
    if(running)
        shutdown();
}

void CompileContext::dump(){
    printf("CompileContext final setup\nLocals\n");
    printf("Closure count %d, Param count %d\n",closureCt,paramCt);
    for(int i=0;i<localTokenCt;i++){
        printf("   %20s %s %4d\n",
               localTokens[i],
               (localsClosed&(1<<i))?"C":" ",
               localIndices[i]);
    }
}


void Angort::shutdown(){
    ArrayListIterator<LibraryDef *>iter(libs);
    
    for(iter.first();!iter.isDone();iter.next()){
        LibraryDef *lib = *(iter.current());
        NativeFunc shutdownFunc = lib->shutdownfunc;
        if(shutdownFunc)
            (*shutdownFunc)(this);
    }
    
    Type::clearList();
    SymbolType::deleteAll();
    running = false;
}

void Angort::showop(const Instruction *ip,const Instruction *base){
    if(!base)base=wordbase;
    char buf[128];
    Value tmp;
    printf("%8p [%s:%d] : %04d : %s (%d) ",
           base,
#if SOURCEDATA
           ip->file,ip->line,
#else
           "?",0,
#endif
           (int)(ip-base),
           opcodenames[ip->opcode],
           ip->opcode);
    switch(ip->opcode){
    case OP_FUNC:
        Types::tNative->set(&tmp,ip->d.func);
        printf(" (%s)",names.getNameByValue(&tmp,buf,128));
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
        printf("(%s)",names.getName(ip->d.i));
        break;
    case OP_CLOSURESET:
    case OP_CLOSUREGET:
    case OP_LOCALSET:
    case OP_LOCALGET:
        printf("(%d)",ip->d.i);break;
    case OP_PROPSET:
    case OP_PROPGET:
        Types::tProp->set(&tmp,ip->d.prop);
        printf(" (%s)",names.getNameByValue(&tmp,buf,128));
        break;
    case OP_LITERALSTRING:
        printf("(%s)",ip->d.s);
        break;
    case OP_LITERALSYMB:
    case OP_HASHGETSYMB:
    case OP_HASHSETSYMB:
        printf("(%d:%s)",ip->d.i,
               Types::tSymbol->getString(ip->d.i));
        break;
    default:break;
    }
    tmp.clr();
    
}

const Instruction *Angort::call(const Value *a,const Instruction *returnip){
    const CodeBlock *cb;
    Type *t;
    
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
    
    //    printf("Locals = %d of which closures = %d\n",cb->locals,cb->closureBlockSize);
    //    printf("Allocating %d stack spaces\n",cb->locals - cb->closureBlockSize);
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
        if(tp){
            // type check
            if(tp != paramval->t){
                // mismatch - special cases of coercion
                if(tp == Types::tFloat && paramval->t==Types::tInteger){
                    // expecting a float, got an int.
                    Types::tFloat->set(&tmpval,paramval->v.i);
                    paramval = &tmpval;
                } else if(tp == Types::tInteger && paramval->t==Types::tFloat){
                    Types::tInteger->set(&tmpval,(int)(paramval->v.f));
                    paramval = &tmpval;
                } else
                    throw RUNT(EX_BADPARAM,"").set("Type mismatch: argument %d is %s, expected %s",
                                                   i,paramval->t->name,tp->name);
            }
        }                          
        
        
        
        if(cb->localsClosed & (1<<i)){
            //            printf("Param %d is closed: %s, into closure %d\n",i,paramval->toString().get(),*pidx);
            clos->map[*pidx]->copy(paramval);
        } else {
            //            printf("Param %d is open: %s, into local %d\n",i,paramval->toString().get(),*pidx);
            locals.store(*pidx,paramval);
        }
    }
    stack.drop(cb->params);
    //    if(clos)clos->show("VarStorePostParams");
    
    
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
    
    //    printf("PUSHED closure %s\n",currClosure.toString().get());
    
    f->loopIterCt=loopIterCt;
    loopIterCt=0;
    
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

void Angort::runValue(const Value *v){
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

void Angort::dumpStack(const char *s){
    printf("Stack dump for %s\n",s);
    for(int i=0;i<stack.ct;i++){
        const StringBuffer &b = stack.peekptr(i)->toString();
        printf("  %s\n",b.get());
    }
}

void Angort::ret()
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

void Angort::throwAngortException(int symbol, Value *data){
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
                return;
            }
            // failing that, look for a catch-all.
            offset = h->ffind(CATCHALLKEY);
            if(offset){
                // FOUND IT - deal with it and return, stacking
                // the value and the exception ID.
                pushval()->copy(data);
                Types::tSymbol->set(pushval(),symbol);
                ip = wordbase+*offset;
                return;
            }
            // failing THAT, pop another try-catch set off.
        }
        // didn't find it in the intrafunction stack, need
        // to return from this function and try again.
        ret();
    }
    ip=NULL; // no handler found, abort!
}

void Angort::run(const Instruction *startip){
    ip=startip;
    ipException = NULL;
    
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
                if(debug&1){
                    showop(ip,wordbase);
                    printf(" ST [%d] : ",stack.ct);
                    for(int i=0;i<stack.ct;i++){
                        const StringBuffer &sb = stack.peekptr(i)->toString();
                        printf("%s ",sb.get());
                    }
                    printf("\n");
                }
                if(autoCycleInterval>0 && !--autoCycleCount){
                    autoCycleCount = autoCycleInterval;
                    gc();
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
                case OP_LITERALINT:
                    pushInt(ip->d.i);
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
                    if(cb->closureBlockSize || cb->closureTableSize){
                        Closure *cl = new Closure(currClosure.v.closure); // 1st stage of setup
                        Types::tClosure->set(a,cl);
                        a->v.closure->init(cb); // 2nd stage of setup
                    } else
                        Types::tCode->set(a,cb);
                    ip++;
                }
                    break;
                case OP_CLOSUREGET:
                    if(currClosure.t != Types::tClosure)throw WTF;
                    a = currClosure.v.closure->map[ip->d.i];
                    //                currClosure.v.closure->show("VarGet");
                    stack.pushptr()->copy(a);
                    ip++;
                    break;
                case OP_CLOSURESET:
                    if(currClosure.t != Types::tClosure)throw WTF;
                    //                currClosure.v.closure->show("VarSet");
                    a = currClosure.v.closure->map[ip->d.i];
                    a->copy(stack.popptr());
                    ip++;
                    break;
                case OP_GLOBALSET:
                    // SNARK - combine with consts
                    a = popval();
                    names.getVal(ip->d.i)->copy(a);
                    ip++;
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
                    a = names.getVal(ip->d.i);
                    if(a->t->isCallable()){
                        Closure *clos;
                        Value vv;
                        // here, we construct a closure block for the global if
                        // required. This results in a new value being created which
                        // goes into the frame.
                        if(a->t == Types::tCode){
                            const CodeBlock *cb = a->v.cb;
                            if(cb->closureBlockSize || cb->closureTableSize){
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
                    break;
                case OP_GLOBALGET:
                    // like the above but does not run a codeblock
                    a = names.getVal(ip->d.i);
                    b = stack.pushptr();
                    b->copy(a);
                    ip++;
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
                    if(popInt())
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
                    if(popInt()){
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
                    Types::tInteger->set(a,!a->toInt());
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
                case OP_DOT:{
                    a = popval();
                    const StringBuffer &sb = a->toString();
                    puts(sb.get());
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
                case OP_LIBRARY:
                    // load a plugin (a shared library). Will
                    // push the plugin's namespace ID ready for
                    // import or list-import
                    pushInt(plugin(popval()->toString().get()));
                    ip++;    
                    break;
                case OP_IMPORT:
                    // the stack will have one of two configurations:
                    // (int --) will import everything (i.e. add the
                    // namespace to the "imported namespaces list")
                    // while (int list --) will only import the listed
                    // symbols (i.e. copy them into the default space)
                    if(stack.peekptr()->t==Types::tInteger)
                        names.import(popInt(),NULL);
                    else if(stack.peekptr()->t==Types::tList){
                        ArrayList<Value> *lst = Types::tList->get(stack.popptr());
                        names.import(popInt(),lst);
                    } else
                        throw SyntaxException("expected package list or package in import");
                    ip++;
                    break;
                case OP_DEF:{
                    const StringBuffer& sb = popString();
                    if(names.isConst(sb.get(),false))
                        throw AlreadyDefinedException(sb.get());
                    int idx = ip->d.i ? names.addConst(sb.get()):names.add(sb.get());
                    names.getVal(idx)->copy(popval());
                    ip++;
                    break;
                }
                case OP_CONSTEXPR:
                    pushval()->copy(ip->d.constexprval);
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
                    throwAngortException(a->v.i,b);
                    if(!ip){
                        // IP has returned NULL, which means we couldn't find an Angort
                        // handler (to which IP would point otherwise)
                        const StringBuffer &sbuf = b->toString();
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
                throwAngortException(e.id,&vvv);
                if(!ip){
                    throw e;
                }
            }
        }
    } catch(Exception e){
        // this is called when the outer handler throws
        // an error, i.e. the error was not handled by an
        // Angort try-catch block.
        // store the exception details
        ipException = ip;
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
    catchstack.pop();
}

void Angort::startDefine(const char *name){
    //    printf("---Now defining %s\n",name);
    int idx;
    if(isDefining())
        throw SyntaxException("cannot define a word inside another");
    if((idx = names.get(name))<0)
        idx = names.add(name); // words are NOT constant; they can be redefined.
    else
        if(names.getEnt(idx)->isConst)
            throw SyntaxException("").set("cannot redefine constant '%s'",name);
    wordValIdx = idx;
}


void Angort::endDefine(CompileContext *c){
    if(!isDefining())
        throw SyntaxException("not defining a word");
    
    // get the codeblock out of the context and set it up.
    CodeBlock *cb = c->cb;
    //    c->dump();
    cb->setFromContext(c);
    Value *wordVal = names.getVal(wordValIdx);
    
    Types::tCode->set(wordVal,cb);
    names.setSpec(wordValIdx,c->spec);
    wordValIdx = -1;
}

void Angort::compileParamsAndLocals(){
    
    if(!isDefining() && !inSubContext())
        throw SyntaxException("cannot use [] outside a word definition or code literal.");
    if(context->getCodeSize()!=0)
        throw SyntaxException("[] must be come first in a word definition or code literal");
    
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
    for(ClosureListEnt *p=closureList;p;p=p->next){
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
        //        printf("Closure table for context %p: Setting entry %d to %d/%d\n",this,(int)(t-table),level,t->idx);
        t++;
    }
    return table;
}

void CompileContext::closeAllLocals(){
    //    printf("YIELD DETECTED: CLOSING ALL LOCALS\n");
    for(int i=0;i<localTokenCt;i++){
        convertToClosure(localTokens[i]);
    }
}

void CompileContext::convertToClosure(const char *name){
    // find which local this is
    int previdx;
    for(previdx=0;previdx<localTokenCt;previdx++)
        if(!strcmp(localTokens[previdx],name))break;
    if(previdx==localTokenCt)throw WTF;
    // got it. Now set this as a closure.
    //    printf("Converting %s into closure\n",name);
    if((1<<previdx) & localsClosed){
        //        printf("%s is already closed\n",name);
        return; // it's already converted.
    }
    
    
    localsClosed |= 1<<previdx; // set it to be closed
    
    int localIndex = localIndices[previdx];
    
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
    addClosureListEnt(cb,closureCt++);
    // but there may be closures here already, so set the local
    // index for the closures to the most recent entry in that table.
    localIndices[previdx] = closureListCt-1;
    
    // convert all access of the local into the closure
    Instruction *inst = compileBuf;
    //    printf("Beginning scan to convert local %d into closure %d\n",
    //           localIndex,localIndices[previdx]);
    for(int i=0;i<compileCt;i++,inst++){
        //        char buf[1024];
        //        printf("   %s  %d\n",inst->getDetails(buf,1024),inst->d.i);
        if(inst->opcode == OP_LOCALGET && inst->d.i == localIndex) {
            inst->opcode = OP_CLOSUREGET;
            //            printf("Rehashing to %d\n",localIndices[previdx]);
            inst->d.i = localIndices[previdx];
        }
        if(inst->opcode == OP_LOCALSET && inst->d.i == localIndex) {
            inst->opcode = OP_CLOSURESET;
            //            printf("Rehashing to %d\n",localIndices[previdx]);
            inst->d.i = localIndices[previdx];
        }
    }
    //    printf("Ending scan\n");
    
    // now decrement all indices of locals greater than this.
    // Firstly do this in the table.
    
    //    printf("Now decrementing subsequent tokens\n");
    for(int i=0;i<localTokenCt;i++){
        //        printf("Token %d : %s\n",i,localTokens[i]);
        if(!isClosed(i) && localIndices[i]>localIndex){
            localIndices[i]--;
            //            printf("  decremented to %d\n",localIndices[i]);
        }
    }
    
    // Then do it in the code generated thus far.
    
    inst = compileBuf;
    //    printf("Now decrementing local accesses in generated code\n");
    for(int i=0;i<compileCt;i++,inst++){
        //        char buf[1024];
        //        printf("   %s  %d\n",inst->getDetails(buf,1024),inst->d.i);
        if((inst->opcode == OP_LOCALGET || inst->opcode == OP_LOCALSET) &&
           inst->d.i > localIndex){
            inst->d.i--;
            //            printf("  decremented to %d\n",inst->d.i);
        }
    }
}

int CompileContext::findOrCreateClosure(const char *name){
    // first, scan all functions above this for a local variable.
    int localIndexInParent=-1;
    CompileContext *parentContainingVariable;
    
    //    printf("Making closure list. Looking for %s\n",name);
    for(parentContainingVariable=parent;parentContainingVariable;
        parentContainingVariable=parentContainingVariable->parent){
        //        printf("   Looking in %p\n",parentContainingVariable);
        if((localIndexInParent = parentContainingVariable->getLocalToken(name))>=0){
            // got it. If not already, turn it into a closure (which will add it to
            // the closure table of that function)
            //            printf("     Got it.\n");
            if(!parentContainingVariable->isClosed(localIndexInParent)){
                //                printf("     Got it, not closed, closing.\n");
                parentContainingVariable->convertToClosure(name);
            }
            break;
        }
    }
    if(localIndexInParent<0)return -1; // didn't find it
    
    // get the index within the closure block
    localIndexInParent = parentContainingVariable->getLocalIndex(localIndexInParent);
    
    // scan the closure table to see if we have this one already
    int nn=0;
    for(ClosureListEnt *p=closureList;p;p=p->next,nn++){
        if(p->c == parentContainingVariable->cb && p->i == localIndexInParent)
            return nn;
    }
    // if not, add it.
    return addClosureListEnt(parentContainingVariable->cb,localIndexInParent);
}



void Angort::endPackageInScript(){
    // see the similar code below in include().
    // pop the namespace stack
    int idx=names.pop();
    pushInt(idx);
    names.setPrivate(false); // and clear the private flag
}

void Angort::include(const char *filename,bool isreq){
    // find the file
    
    const char *fh = findFile(filename);
    if(!fh)
        throw FileNotFoundException(filename);
    
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
        pushInt(idx);
    }
    
    names.setPrivate(false); // and clear the private flag
    
    
    free(path);
    if(fchdir(oldDir))
        throw RUNT(EX_BADINC,"unable to reset directory in 'include'");
    close(oldDir);
}

void Angort::feed(const char *buf){
    
    ipException = NULL;
    resetStop();
    callingInstance=this;
    
    if(printLines)
        printf("%d >>> %s\n",lineNumber,buf);
    
    if(!isDefining() && context && !inSubContext()) // make sure we're reset unless we're compiling or subcontexting
        context->reset(NULL,&tok);
    
    strcpy(lastLine,buf);
    tok.settrace((debug&2)?true:false);
    tok.reset(buf);
    // the tokeniser will reset its idea of the line number,
    // because we reset it at the start of all input.
    tok.setline(lineNumber);
    
    if(hereDocEndString!=NULL){
        if(!strcmp(buf,hereDocEndString)){
            hereDocEndString = NULL;
            
            // compile and if necessary run the code to stack the string
            compile(OP_LITERALSTRING)->d.s = strdup(hereDocString);
            free(hereDocString);
            hereDocString=NULL;
            if(!isDefining() && !inSubContext()){
                compile(OP_END);run(context->getCode());
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
    
    int here; // instruction index variable used in different ways
    try {
        for(;;){
            
            int t = tok.getnext();
            switch(t){
            case T_INCLUDE:{
                char buf[1024];
                // will recurse
                if(!tok.getnextstring(buf))
                    throw SyntaxException("expected a filename after 'include'");
                //                if(tok.getnext()!=T_END)
                //                    throw SyntaxException("include must be at end of line");
                
                include(buf,false);
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
                include(buf,true);
                break;
            }
            case T_PRIVATE:
                names.setPrivate(true);
                break;
            case T_PUBLIC:
                names.setPrivate(false);
                break;
            case T_PACKAGE:{
                // start a new package.
                char buf[256];
                if(!tok.getnextident(buf))
                    throw SyntaxException("expected a package name");
                int idx = names.create(buf);
                // stack it, we're now defining things in
                // this package and will be until fileFeed() returns
                // in include()
                names.push(idx);
                break;
            }
            case T_IMPORT:
                compile(OP_IMPORT);
                break;
            case T_LIBRARY:
                compile(OP_LIBRARY);
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
                //                printf("defined %s - %d ops\n",defineName,context->getCodeSize());
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
            case T_FLOAT:
                compile(OP_LITERALFLOAT)->d.f = tok.getfloat();
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
            case T_IDENT:
                {
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
            case T_QUESTION:
                {
                    if(tok.getnext()!=T_IDENT)
                        throw SyntaxException("expected identifier after ?");
                    if((t = context->getLocalToken(tok.getstring()))>=0){
                        // it's a local variable
                        compile(context->isClosed(t) ? OP_CLOSUREGET : OP_LOCALGET)->d.i=
                              context->getLocalIndex(t);
                    } else if((t=context->findOrCreateClosure(tok.getstring()))>=0){
                        // it's a variable defined in a function above
                        compile(OP_CLOSUREGET)->d.i = t;
                    } else if((t = names.get(tok.getstring()))>=0){
                        // it's a global
                        Value *v = names.getVal(t);
                        if(v->t == Types::tProp){
                            // it's a property
                            compile(OP_PROPGET)->d.prop = v->v.property;
                        } else {
                            // it's a global; use it - but don't call it if it's a function
                            compile(OP_GLOBALGET)->d.i = t;
                        }
                    } else if(isupper(*tok.getstring())){
                        // if it's upper case, immediately define as a global
                        compile(OP_GLOBALGET)->d.i=
                              findOrCreateGlobal(tok.getstring());
                    } else {
                        throw SyntaxException(NULL)
                              .set("unknown variable: %s",tok.getstring());
                    }
                    break;
                }
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
            case T_PLING: {
                if(tok.getnext()!=T_IDENT)
                    throw SyntaxException("expected identifier after !");
                if((t = context->getLocalToken(tok.getstring()))>=0){
                    // it's a local variable
                    compile(context->isClosed(t) ? OP_CLOSURESET : OP_LOCALSET)->d.i=
                          context->getLocalIndex(t);
                } else if((t=context->findOrCreateClosure(tok.getstring()))>=0){
                    // it's a variable defined in a function above
                    compile(OP_CLOSURESET)->d.i = t;
                } else if((t = names.get(tok.getstring()))>=0){
                    // it's a global
                    Value *v = names.getVal(t);
                    if(v->t == Types::tProp){
                        // it's a property
                        compile(OP_PROPSET)->d.prop = v->v.property;
                    } else {
                        if(names.getEnt(t)->isConst)
                            throw RUNT(EX_SETCONST,"").set("attempt to set constant %s",tok.getstring());
                        // it's a global; use it
                        compile(OP_GLOBALSET)->d.i = t;
                    }
                } else if(isupper(*tok.getstring())){
                    // if it's upper case, immediately define as a global
                    compile(OP_GLOBALSET)->d.i=
                          findOrCreateGlobal(tok.getstring());
                } else {
                    throw SyntaxException(NULL)
                          .set("unknown variable: %s",tok.getstring());
                }
                break;
            }
            case T_GLOBAL:
                if(tok.getnext()!=T_IDENT)
                    throw SyntaxException(NULL)
                      .set("expected an identifier, got %s",tok.getstring());
                
                // "false" here so we don't look in imported namespaces!
                
                if(names.isConst(tok.getstring(),false))
                    throw AlreadyDefinedException(tok.getstring());
                names.add(tok.getstring());
                break;
            case T_DOUBLEANGLEOPEN:
            case T_OPREN:// open lambda
                //                printf("---Pushing: context is %p[cb:%p], ",context,context->cb);
                pushCompileContext();
                //                printf("lambda context is %p[cb:%p]\n",context,context->cb);
                break;
            case T_CPREN: // close lambda and stack a code literal
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
                    runValue(vv);
                    
                    // we don't need the codeblock any more (note,
                    // if we ever GC codeblocks this will free twice)
                    delete lambdaContext->cb;
                    lambdaContext->reset(NULL,&tok);
                    // get the value that codeblock had
                    vv->copy(popval());
                    compile(OP_CONSTEXPR)->d.constexprval=vv;
                }
                break;
            case T_END:
                // just return if we're still defining
                if(!isDefining() && !inSubContext()){
                    context->checkStacksAtEnd(); // check dangling constructs
                    // otherwise run the buffer we just made
                    compile(OP_END);
                    run(context->getCode());
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
            default:
                throw SyntaxException(NULL).set("unhandled token: %s",
                                                tok.getstring());
            }
        }
    } catch(Exception e){
        clearAtEndOfFeed();
        throw e; // and rethrow
    }
    lineNumber++;
}



void Angort::clearAtEndOfFeed(){
    //    printf("Clearing at end of feed\n");
    // make sure we tidy up any state
    contextStack.clear(); // clear the context stack
    context = contextStack.pushptr();
    context->reset(NULL,&tok); // reset the old context
    wordValIdx=-1;
    // make sure the return stack gets cleared otherwise
    // really strange things can happen on the next processed
    // line
    
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

void Angort::disasm(const char *name){
    int idx = names.get(name);
    if(idx<0)
        throw RUNT(EX_NOTFOUND,"unknown function");
    Value *v = names.getVal(idx);
    if(v->t != Types::tCode)
        throw RUNT(EX_NOTFUNC,"not a function");
    
    const CodeBlock *cb = v->v.cb;
    const Instruction *ip = cb->ip;
    const Instruction *base = ip;
    for(;;){
        int opcode = ip->opcode;
        showop(ip++,base);
        printf("\n");
        if(opcode == OP_END)break;
    }
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
void Angort::dumpFrame(){
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
    
    // make the namespace. Multiple imports into the same one
    // are permitted.
    Namespace *sp = names.getSpaceByName(lib->name,true);
    
    // register the words
    for(int i=0;;i++){
        if(!lib->wordList[i].name)break;
        int id = sp->addConst(lib->wordList[i].name,false);
        Value *v = sp->getVal(id);
        Types::tNative->set(v,lib->wordList[i].f);
        sp->setSpec(id,lib->wordList[i].desc);
    }
    
    
    *libs->append() = lib;
    
    if(lib->initfunc){
        (*lib->initfunc)(this);
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
    
    if(import)
        names.import(sp->idx,NULL);
    
    return sp->idx;
    
}

void Angort::storeTrace(){
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

void Angort::printAndDeleteStoredTrace(){
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

void Angort::resetAutoComplete(){
    // build the new autocomplete list
    acList->clear();
    // first, add the tokens
    for(int i=0;tokens[i].word;i++){
        *acList->append()=strdup(tokens[i].word);
    }
    // then add the default namespace
    Namespace *ns = names.getSpaceByIdx(stdNamespace);
    for(int i=0;i<ns->count();i++){
        *acList->append()=strdup(ns->getName(i));
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
    qsort(data,ct,sizeof(Value),arrayCmp);
}

}
