/**
 * @file 
 *
 * 
 * @author $Author$
 * @date $Date$
 */

#define ANGORT_VERSION 222

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

WORDS(std);WORDS(coll);WORDS(string);

int Angort::getVersion(){
    return ANGORT_VERSION;
}


Angort::Angort() {
    Types::createTypes();
    // create and set default namespace
    names.create("default",true);
    lineNumber=1;
    definingPackage=false;
    
    tok.init();
    tok.setname("<stdin>");
    tok.settokens(tokens);
    tok.setcommentlinesequence("#");
    
    REGWORDS((*this),std);
    REGWORDS((*this),coll);
    REGWORDS((*this),string);
    
    debug=false;
    printLines=false;
    emergencyStop=false;
    assertDebug=false;
    assertNegated=false;
    wordValIdx=-1;
    barewords=false;
    
    /// create the default, root compilation context
    context = contextStack.pushptr();
    context->reset(NULL,&tok);
}


void Angort::showop(const Instruction *ip,const Instruction *base){
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
        printf(" (%s)",funcs.getKey(ip->d.func));
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
        printf("(%p)",closureTable+ip->d.i);
        break;
    case OP_PROPSET:
    case OP_PROPGET:
        printf("(%s)",props.getKey(ip->d.prop));
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
    
}

const Instruction *Angort::call(const Value *a,const Instruction *returnip){
    Closure *closure;
    const CodeBlock *cb;
    Value *v;
    Type *t;
    
    t=a->getType();
    
    /// very simple to handle a native.
    if(t==Types::tNative) {
        callPlugin(Types::tNative->get(a));
        return returnip;
    }
    
    GarbageCollected *toPushOntoGCRStack=NULL;
    
    closureStack.push(closureTable); // we *might* be changing the closure table
    
    
    locals.push(); // switch to new locals frame
    // get the codeblock, check the type, and in
    // the case of a closure, bind the closed locals
    
    if(t==Types::tCode){
        cb = a->v.cb;
        locals.allocLocalsAndPopParams(cb->locals,
                                       cb->params,
                                       &stack);
        if(!cb->ip)
            throw RUNT("call to a word with a deferred definition");
    } else if(t==Types::tClosure) {
        closure = a->v.closure;
        // we need to push this and increment refct so that it won't
        // be deleted by overwrite of the old stack entry inside the
        // closure (see test cases)
        toPushOntoGCRStack=closure;
        closure->incRefCt();
        
        cb = closure->codeBlockValue.v.cb;
        locals.allocLocalsAndPopParams(cb->locals,
                                       cb->params,
                                       &stack);
        if(!cb->closureMap)
            throw RUNT("weird - the closure's codeblock has no closure map");
        // set the closure table we're using
        closureTable = closure->table;
    } else {
        throw RUNT("").set("attempt to 'call' something that isn't code, it's a %s",t->name);
    }
    
    gcrstack.push(toPushOntoGCRStack);
    
    // stack the return ip and return the new one.
    // This might be null, and in that case we ignore it when we pop it.
    rstack.push(returnip);
    
    debugwordbase = cb->ip;
    return cb->ip;
}

void Angort::runValue(const Value *v){
    if(!emergencyStop){
        const Instruction *oldbase=debugwordbase;
        const Instruction *ip=call(v,NULL);
        run(ip);
        debugwordbase=oldbase;
    }
//    locals.pop(); 
}

void Angort::dumpStack(const char *s){
    char buf[256];
    printf("Stack dump for %s\n",s);
    for(int i=0;i<stack.ct;i++){
        stack.peekptr(i)->toString(buf,256);
        printf("  %s\n",buf);
    }
}

const Instruction *Angort::ret()
{
    if(rstack.isempty()){
        // sanity check --- the closure stack should also be zero
        // when the return stack is empty
        if(!closureStack.isempty())
            throw RUNT("closure/return stack mismatch");
        return NULL;
    } else {
        ip = rstack.pop();
        if(GarbageCollected *gc = gcrstack.pop()){
            if(gc->decRefCt())
                delete gc;
        }
        closureTable = closureStack.pop();
        //                printf("CLOSURE SNARK POP\n");
        debugwordbase = ip;
        locals.pop();
        return ip;
    }
}

void Angort::run(const Instruction *ip){
    
    ipException = NULL;
    
    Value *a, *b, *c;
    debugwordbase = ip;
    const CodeBlock *cb;
    
    try {
        for(;;){
            if(emergencyStop){
                ip = ret();
                if(!ip)
                    return;
            }
               
            int opcode = ip->opcode;
            if(debug&1){
                showop(ip,debugwordbase);
                printf(" ST [%d] : ",stack.ct);
                for(int i=0;i<stack.ct;i++){
                    printf("%s ",stack.peekptr(i)->toString(strbuf1,1024));
                }
                printf("\n");
            }
            
            
            
            switch(opcode){
            case OP_EQUALS:      case OP_ADD:            case OP_MUL:          case OP_DIV:
            case OP_SUB:         case OP_NEQUALS:        case OP_AND:          case OP_OR:
            case OP_GT:          case OP_LT:             case OP_MOD:
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
            case OP_LITERALCODE:
                cb = ip->d.cb;
                // if there is a closure map, we'll need to build the closure and push that
                if(cb->closureMap){
                    // building the closure
                    int size = cb->closureMapCt;
                    Value *table = new Value[size];
                    
                    for(int i=0;i<size;i++){
                        // copy values from the parent, creating context into the closure
                        if(cb->closureMap[i].isLocalInParent)
                            table[i].copy(locals.get(cb->closureMap[i].parent));
                        else
                            table[i].copy(closureTable+cb->closureMap[i].parent);
                    }
                    Closure *closure = new Closure(cb,size,table);
                    Types::tClosure->set(stack.pushptr(),closure);
                } else
                    // otherwise we just push the codeblock
                    Types::tCode->set(stack.pushptr(),cb);
                ip++;
                break;
            case OP_CLOSUREGET:
                a = stack.pushptr();
                a->copy(closureTable+ip->d.i);
                ip++;
                break;
            case OP_CLOSURESET:
                a = stack.popptr();
                closureTable[ip->d.i].copy(a);
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
                (*ip->d.func)(this);
                ip++;
                break;
            case OP_GLOBALDO:
                a = names.getVal(ip->d.i);
                if(a->t->isCallable()){
                    ip = call(a,ip+1);
                } else {
                    b = stack.pushptr();
                    b->copy(a);
                    ip++;
                }
                break;
            case OP_GLOBALGET:
                // like the above but does not run a codeblock/closure
                a = names.getVal(ip->d.i);
                b = stack.pushptr();
                b->copy(a);
                ip++;
                break;
            case OP_CALL:
                // easy as this - pass in the value
                // and the return ip, get the new ip
                // out.
                {
                    // it's OK to pop a closure here, it only becomes a problem if
                    // we overwrite it.
                    ip=call(popval(),ip+1); // this JUST CHANGES THE IP AND STACKS STUFF.
                }
                break;
            case OP_END:
            case OP_STOP:
                ip=ret();
                if(!ip)
                    return;
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
                // fall through
            case OP_JUMP:
                ip+=ip->d.i;
                break;
            case OP_IFLEAVE:
                if(popInt()){
                    loopIterStack.popptr()->clr();
                    ip+=ip->d.i;
                } else
                    ip++;
                break;
            case OP_LOOPSTART:
                // start of an infinite loop, so push a None iterator
                a = loopIterStack.pushptr();
                a->clr();
                ip++;
                break;
            case OP_ITERSTART:{
                a = stack.popptr(); // the iterable object
                // we make an iterator and push it onto the iterator stack
                b = loopIterStack.pushptr();
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
                puts(a->toString(strbuf1,1024));
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
            case OP_CLOSELIST:
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
                        throw RUNT("attempt to set value in non-hash or list");
                } else {
                    b = Types::tList->get(b)->append();
                    b->copy(a);
                }
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
                    throw SyntaxException("expected package list or package im import");
                ip++;
                break;
                
            default:
                throw RUNT("unknown opcode");
            }
            
        }
    } catch(Exception e){
        // store the exception details
        ipException = ip;
        // destroy any iterators left lying around
        while(!loopIterStack.isempty())
            loopIterStack.popptr()->clr();
        throw e;
    }
}

void Angort::startDefine(const char *name){
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
    
    CodeBlock *cb = new CodeBlock(c);
    cb->spec = c->getSpec() ? strdup(c->getSpec()) : NULL;    
    
    Value *wordVal = names.getVal(wordValIdx);
    Types::tCode->set(wordVal,cb);
    wordValIdx = -1;
}

void Angort::endDefine(Instruction *i){
    if(!isDefining())
        throw SyntaxException("not defining a word");
    
    CodeBlock *cb = new CodeBlock(i);
    Value *wordVal = names.getVal(wordValIdx);
    Types::tCode->set(wordVal,cb);
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
    
    for(;;){
        int t = tok.getnext();
        Instruction *i;
        
        switch(t){
        case T_IDENT:
            // add a new local token; and
            // add to the pop count if it's a parameter
            context->addLocalToken(tok.getstring());
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
            return; // and exit
        }
    }
}

bool Angort::fileFeed(const char *name,bool rethrow){
    definingPackage=false;
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
            throw Exception().set("cannot open %s",name);
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

const Instruction *Angort::compile(const char *s){
    RUNT("no longer supported");
    /*
    // trick the system into thinking we're defining a colon
    // word for one line only
    defining = true'
    feed(s);
    compile(OP_END); // make sure it's terminated
    
    // get the code, copy it, reset the context and return.
    int ct = context->getCodeSize();
    Instruction *ip = context->getCode();
    
    Instruction *buf = new Instruction[ct];
    memcpy(buf,ip,ct*sizeof(Instruction));
    
    context->reset(NULL);
    defining=false; // must turn it off!
    return buf;
     */    
    return NULL;
}

void Angort::include(const char *fh,bool isreq){
    int oldDir = open(".",O_RDONLY); // get the FD for the current directory so we can go back
    
    // first, find the real path of the file
    char *path=realpath(fh,NULL);
    if(!path)
        throw Exception().set("cannot open file : %s",fh);
    
    // now get the directory separator
    char *file = strrchr(path,'/');
    *file++ = 0; // and the file name
    
    // change to that directory, so all future reads are relative to there
    chdir(path);
    
    TokeniserContext c;
    tok.saveContext(&c);
    fileFeed(file);
    tok.restoreContext(&c);
    if(names.getStackTop()>=0){
        // pop the namespace stack
        int idx=names.pop();
        if(isreq){
            // push the idx of the package which was defined. 
            // A bit dodgy since this isn't taking place in
            // a code block..
            pushInt(idx);
        }
    }
    names.setPrivate(false); // and clear the private flag
    
    
    free(path);
    fchdir(oldDir);
    close(oldDir);
}

void Angort::feed(const char *buf){
    ipException = NULL;
    resetStop();
    
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
    
    int t,here;
    Instruction *code;
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
                if(definingPackage)
                    throw SyntaxException("already in a package");
                definingPackage=true;
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
            case T_LIBRARY:{
                // load a plugin (a shared library). Will
                // push the plugin's namespace ID ready for
                // import or list-import
                char buf[1024];
                // will recurse
                if(!tok.getnextstring(buf))
                    throw FileNameExpectedException();
                plugin(buf);
                break;
            }
            case T_BACKTICK:{
                char buf[256];
                if(!tok.getnextidentorkeyword(buf))
                    throw FileNameExpectedException();
                compile(OP_LITERALSYMB)->d.i=Types::tSymbol->getSymbol(buf);
                break;
            }
            case T_CONST: // const syntax = <val> const <ident>
                {
                    if(isDefining())
                        throw SyntaxException("'const' not allowed in a definition");
                    if(tok.getnext()!=T_IDENT)
                        throw SyntaxException("expected an identifier");
                    
                    if(names.get(tok.getstring())>=0)
                        throw Exception().set("const %s already set",tok.getstring());
                    
                    int n = names.addConst(tok.getstring());
                    // we write an instruction to 
                    // store this const
                    compile(OP_GLOBALSET)->d.i=n;
                }
                break;
            case T_COLON:
                if(isDefining()){
                    // the only valid use of ":" in a definition is in a specstring.
                    char spec[1024];
                    if(!tok.getnextstring(spec))
                        throw SyntaxException("expected spec string after second ':' in definition");
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
            case T_IDENT:
                {
                    char *s = tok.getstring();
                    if((t = names.get(s))>=0){
                        compile(OP_GLOBALDO)->d.i = t;
                    } else if(NativeFunc f = getFunc(s)){
                        compile(OP_FUNC)->d.func = f;
                    } else if(barewords){
                        compile(OP_LITERALSYMB)->d.i=
                             Types::tSymbol->getSymbol(tok.getstring());
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
            case T_QUESTION:
                {
                    if(tok.getnext()!=T_IDENT){
                        char buf[256];
                        switch(tok.getcurrent()){
                        case T_BACKTICK:
                            if(!tok.getnextident(buf))
                                throw SyntaxException("expected a symbol after backtick");
                            compile(OP_HASHGETSYMB)->d.i=Types::tSymbol->getSymbol(buf);
                            break;
                        default:
                            throw SyntaxException("expected an identifier");
                        }
                    } else if((t = context->getLocalToken(tok.getstring()))>=0){
                        // if we couldn't find it as a local, try to find it as a 
                        // local in the parent context. If that succeeds, we will have
                        // created a local for it.
                        compile(OP_LOCALGET)->d.i=t;
                    } else if((t = context->findOrAttemptCreateClosure(tok.getstring()))>=0){
                        // we managed to create a closure from here to upstairs,
                        // so store the closure index
                        compile(OP_CLOSUREGET)->d.i=t;
                    } else if(Property *p = getProp(tok.getstring())){
                        compile(OP_PROPGET)->d.prop = p;
                    } else if((t = names.get(tok.getstring()))>=0){
                        // it's a global; use it - but don't call it if it's a function
                        compile(OP_GLOBALGET)->d.i = t;
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
            case T_PLING:
                {
                    if(tok.getnext()!=T_IDENT){
                        char buf[256];
                        switch(tok.getcurrent()){
                        case T_BACKTICK:
                            if(!tok.getnextident(buf))
                                throw SyntaxException("expected a symbol after backtick");
                            compile(OP_HASHSETSYMB)->d.i=Types::tSymbol->getSymbol(buf);
                            break;
                        case T_EQUALS:
                            compile(OP_NEQUALS);
                            break;
                        default:
                            throw SyntaxException("expected an identifier");
                        }
                    }else if((t = context->getLocalToken(tok.getstring()))>=0){
                        // if we couldn't find it as a local, try to find it as a 
                        // local in the parent context. If that succeeds, we will have
                        // created a local for it.
                        compile(OP_LOCALSET)->d.i=t;
                    } else if((t = context->findOrAttemptCreateClosure(tok.getstring()))>=0){
                        // we managed to create a closure from here to upstairs,
                        // so store the closure index
                        compile(OP_CLOSURESET)->d.i=t;
                    } else if(Property *p = getProp(tok.getstring())){
                        compile(OP_PROPSET)->d.prop = p;
                    } else if((t = names.get(tok.getstring()))>=0){
                        if(names.getEnt(t)->isConst)
                            throw RUNT("").set("attempt to set constant %s",tok.getstring());
                        // it's a global; use it
                        compile(OP_GLOBALSET)->d.i = t;
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
                
                if(names.get(tok.getstring())<0)// ignore multiple defines
                    names.add(tok.getstring());
                break;
            case T_OPREN:// open lambda
                //                printf("Pushing: context is %p, ",context);
                pushCompileContext();
                //                printf("lambda context is %p\n",context);
                break;
            case T_CPREN: // close lambda and stack a code literal
                {
                    if(!inSubContext())
                        throw SyntaxException("')' not inside a code literal");
                    
                    compile(OP_END); // finish the compile
                    CompileContext *lambdaContext = popCompileContext();
                    
                    // here, we compile LITERALCODE word with a codeblock created
                    // from the context. This will contain a closure map if it was
                    // required.
                    
                    compile(OP_LITERALCODE)->d.cb = new CodeBlock(lambdaContext);
                    lambdaContext->reset(NULL,&tok);
                }
                break;
            case T_END:
                // just return if we're still defining
                if(!isDefining() && !inSubContext()){
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
    // make sure we tidy up any state
    contextStack.clear(); // clear the context stack
    context = contextStack.pushptr();
    context->reset(NULL,&tok); // reset the old context
    wordValIdx=-1;
    // make sure the return stack gets cleared otherwise
    // really strange things can happen on the next processed
    // line
    rstack.clear(); 
    // destroy any iterators left lying around
    while(!loopIterStack.isempty())
        loopIterStack.popptr()->clr();
    closureStack.clear();
}    

void Angort::disasm(const char *name){
    int idx = names.get(name);
    if(idx<0)
        throw RUNT("unknown function");
    Value *v = names.getVal(idx);
    if(v->t != Types::tCode)
        throw RUNT("not a function");
          
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
    int idx;
    const char *spec;
    
    if((idx=names.get(s))>=0){
        Value *v = names.getVal(idx);
        if(v->t != Types::tCode)
            return "<not a function>";
        else
            return v->v.cb->spec;
    } else if(spec=getFuncSpec(s)){
        return spec;
    } else if(spec=getPropSpec(s)){
        return spec;
    }
    return NULL;
}

void Angort::list(){
    printf("GLOBALS:\n");
    names.list();
    
    StringMapIterator<Module *> iter(&modules);
    for(iter.first();!iter.isDone();iter.next()){
        const char *name = iter.current()->key;
        Module *m = iter.current()->value;
        
        if(m->funcs.count()>0){
            printf("MODULE %s WORDS:\n",*name?name:"(none)");
            m->funcs.listKeys();
        }
        if(m->props.count()>0){
            printf("MODULE %s PROPS:\n",*name?name:"(none)");
            m->props.listKeys();
        }
    }
}

Module *Angort::splitFullySpecified(const char **name){
    char *dollar;
    if(dollar=strchr((char *)*name,'$')){
        char buf[32];
        if(dollar- *name > 32){
            throw RUNT("namespace name too long");
        }
        strncpy(buf,*name,dollar- *name);
        buf[dollar- *name]=0;
        
        Module *m = modules.get(buf);
        if(!m)
            throw RUNT("").set("module not found: %s",buf);
        *name = dollar+1;
        return m;
    } else {
        return NULL;
    }
}
    
        

NativeFunc Angort::getFunc(const char *name){
    if(Module *m = splitFullySpecified(&name)){
        return m->funcs.get(name);
    } else {
        return funcs.get(name);
    }
}

Property *Angort::getProp(const char *name){
    if(Module *m = splitFullySpecified(&name)){
        return m->props.get(name);
    } else {
        return props.get(name);
    }
}

const char *Angort::getFuncSpec(const char *name){
    if(Module *m = splitFullySpecified(&name)){
        throw RUNT("fully specified specs not supported");
    } else {
        return funcSpecs.get(name);
    }
}

const char *Angort::getPropSpec(const char *name){
    if(Module *m = splitFullySpecified(&name)){
        throw RUNT("fully specified specs not supported");
    } else {
        return propSpecs.get(name);
    }
}
