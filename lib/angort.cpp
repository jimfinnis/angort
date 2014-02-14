/**
 * @file 
 *
 * 
 * @author $Author$
 * @date $Date$
 */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "angort.h"
#define DEFOPCODENAMES 1
#include "opcodes.h"
#include "tokens.h"

int GarbageCollected::globalCt=0;

WORDS(std);WORDS(lists);



Angort::Angort() {
    Types::createTypes();
    
    tok.init();
    tok.settokens(tokens);
    tok.setcommentlinesequence("#");
    
    REGWORDS((*this),std);
    REGWORDS((*this),lists);
    
    defining = false;
    debug=false;
    printLines=false;
    
    /// create the default, root compilation context
    context = contextStack.pushptr();
    context->reset(NULL);
}


void Angort::showop(const Instruction *ip,const Instruction *base){
    printf("%8p [%8p] : %04d : %s (%d) ",
           base,
           ip,
           (int)(ip-base),
           opcodenames[ip->opcode],
           ip->opcode);
    if(ip->opcode == OP_FUNC){
        printf(" (%s)",funcs.getKey(ip->d.func));
    }
    if(ip->opcode == OP_JUMP||ip->opcode==OP_LEAVE||ip->opcode==OP_ITERLEAVEIFDONE||
       ip->opcode==OP_DECLEAVENEG||ip->opcode==OP_IF){
        printf(" (offset %d)",ip->d.i);
    }
    else if(ip->opcode == OP_WORD){
        printf(" (%s)",words.getName(ip->d.i));
    }
    else if(ip->opcode == OP_GLOBALGET || ip->opcode == OP_GLOBALSET){
        printf(" (index %d)",ip->d.i);
    }
    else if(ip->opcode == OP_CONSTGET || ip->opcode == OP_CONSTSET){
        printf(" (index %d)",ip->d.i);
    }
    else if(ip->opcode == OP_CLOSUREGET || ip->opcode == OP_CLOSURESET){
        printf(" (%p)",closureTable+ip->d.i);
    }
    else if(ip->opcode == OP_PROPGET || ip->opcode == OP_PROPSET){
        printf(" (%s)",props.getKey(ip->d.prop));
    }
    
}

const Instruction *Angort::call(const Value *a,const Instruction *returnip){
    Closure *closure;
    const CodeBlock *cb;
    Value *v;
    Type *t;
    
    GarbageCollected *toPushOntoGCRStack=NULL;
    
    closureStack.push(closureTable); // we *might* be changing the closure table
    
    
    locals.push(); // switch to new locals frame
    // get the codeblock, check the type, and in
    // the case of a closure, bind the closed locals
    t=a->getType();
    
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
        throw RUNT("attempt to 'call' something that isn't code");
    }
    
    gcrstack.push(toPushOntoGCRStack);
    
    // stack the return ip and return the new one.
    // This might be null, and in that case we ignore it when we pop it.
    rstack.push(returnip);
    
    debugwordbase = cb->ip;
    return cb->ip;
}

void Angort::runValue(const Value *v){
    const Instruction *oldbase=debugwordbase;
    const Instruction *ip=call(v,NULL);
    run(ip);
    debugwordbase=oldbase;
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

void Angort::run(const Instruction *ip){
    
    static char strbuf1[1024]; // used for string conversions when required
    static char strbuf2[1024];
    
    Value *a, *b, *c;
    debugwordbase = ip;
    const CodeBlock *cb;
    
    try {
        for(;;){
            int opcode = ip->opcode;
            if(debug){
                showop(ip,debugwordbase);
                printf(" ST [%d] : ",stack.ct);
                for(int i=0;i<stack.ct;i++){
                    stack.peekptr(i)->toString(strbuf1,256);
                    printf("%s ",strbuf1);
                }
                printf("\n");
            }
            
            
            switch(opcode){
            case OP_ADD:
            case OP_MUL:
            case OP_DIV:
            case OP_SUB:
            case OP_EQUALS:
            case OP_NEQUALS:
            case OP_AND:
            case OP_OR:
            case OP_GT:
            case OP_LT:
            case OP_MOD:
                b = popval();
                a = popval();
                if(a->getType() == Types::tString || b->getType() == Types::tString){
                    bool cmp=false;
                    // buffers won't be used if they're already strings
                    const char * p = a->toString(strbuf1,1024);
                    const char * q = b->toString(strbuf2,1024);
                    switch(opcode){
                    case OP_ADD:{
                        c = stack.pushptr();
                        int len = strlen(p)+strlen(q);
                        char *r = Types::tString->allocate(c,len+1);
                        strcpy(r,p);
                        strcat(r,q);
                        break;
                    }
                    case OP_EQUALS:
                        cmp=true;
                        pushInt(!strcmp(p,q));
                        break;
                    case OP_NEQUALS:
                        cmp=true;
                        pushInt(strcmp(p,q));
                        break;
                    case OP_GT:
                        cmp=true;
                        pushInt(strcmp(p,q)>0);
                        break;
                    case OP_LT:
                        cmp=true;
                        pushInt(strcmp(p,q)<0);
                        break;
                    default:throw RUNT("bad operation for strings");
                    }
                    
                }else if(a->getType() == Types::tFloat || b->getType() == Types::tFloat){
                    bool cmp=false;
                    float r;
                    float p = a->toFloat();
                    float q = b->toFloat();
                    switch(opcode){
                    case OP_ADD:
                        r = p+q;break;
                    case OP_SUB:
                        r = p-q;break;
                    case OP_DIV:
                        if(q==0.0f)throw DivZeroException();
                        r = p/q;break;
                    case OP_MUL:
                        r = p*q;break;
                        // "comparisons" go down here - things which
                        // produce integer results even though one operand
                        // is float.
                    case OP_EQUALS:
                        cmp=true;
                        pushInt(p==q);break;
                    case OP_NEQUALS:
                        cmp=true;
                        pushInt(p!=q);break;
                    case OP_AND:
                        cmp=true;
                        pushInt(p&&q);break;
                    case OP_OR:
                        cmp=true;
                        pushInt(p&&q);break;
                    case OP_GT:
                        cmp=true;
                        pushInt(p>q);break;
                    case OP_LT:
                        cmp=true;
                        pushInt(p<q);break;
                    default:
                        throw RUNT("invalid operator for floats");
                    }
                    if(!cmp)
                        pushFloat(r);
                } else {
                    int p,q,r;
                    switch(opcode){
                    case OP_MOD:
                        p = a->toInt();
                        q = b->toInt();
                        r = p%q;break;
                    case OP_ADD:
                        p = a->toInt();
                        q = b->toInt();
                        r = p+q;break;
                    case OP_SUB:
                        p = a->toInt();
                        q = b->toInt();
                        r = p-q;break;
                    case OP_DIV:
                        p = a->toInt();
                        q = b->toInt();
                        if(!q)throw DivZeroException();
                        r = p/q;break;
                    case OP_MUL:
                        p = a->toInt();
                        q = b->toInt();
                        r = p*q;break;
                    case OP_AND:
                        p = a->toInt();
                        q = b->toInt();
                        r = (p&&q);break;
                    case OP_OR:
                        p = a->toInt();
                        q = b->toInt();
                        r = (p||q);break;
                    case OP_GT:
                        p = a->toInt();
                        q = b->toInt();
                        r = (p>q);break;
                    case OP_LT:
                        p = a->toInt();
                        q = b->toInt();
                        r = (p<q);break;
                    case OP_EQUALS:
                        if(a->getType() == Types::tInteger || b->getType() == Types::tInteger)
                            r = a->toInt() == b->toInt();
                        else
                            r = (a->v.s == b->v.s);
                        break;
                    case OP_NEQUALS:
                        if(a->getType() == Types::tInteger || b->getType() == Types::tInteger)
                            r = a->toInt() != b->toInt();
                        else
                            r = (a->v.s != b->v.s);
                        break;
                        
                    }
                    pushInt(r);
                }
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
            case OP_CONSTGET:
                a = stack.pushptr();
                a->copy(consts.get(ip->d.i));
                ip++;
                break;
            case OP_GLOBALGET:
                a = stack.pushptr();
                a->copy(globals.get(ip->d.i));
                ip++;
                break;
            case OP_CONSTSET:
                a=popval();
                consts.get(ip->d.i)->copy(a);
                ip++;
                break;
            case OP_GLOBALSET:
                a = popval();
                globals.get(ip->d.i)->copy(a);
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
            case OP_WORD:
                cb = words.get(ip->d.i)->v.cb;
                rstack.push(ip+1);
                closureStack.push(closureTable);
                gcrstack.push(NULL);
                //            printf("CLOSURE SNARK PUSH\n");
                locals.push();
                locals.allocLocalsAndPopParams(cb->locals,
                                               cb->params,
                                               &stack);
                ip = cb->ip;
                if(!ip)
                    throw RUNT("call to a word with a deferred definition");
                debugwordbase = ip;
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
                if(rstack.isempty()){
                    // sanity check --- the closure stack should also be zero
                    // when the return stack is empty
                    if(!closureStack.isempty())
                        throw RUNT("closure/return stack mismatch");
                    return;
                } else {
                    ip = rstack.pop();
                    if(GarbageCollected *gc = gcrstack.pop())
                        gc->decRefCt();
                    closureTable = closureStack.pop();
                    //                printf("CLOSURE SNARK POP\n");
                    debugwordbase = ip;
                    locals.pop();
                    if(!ip)
                        return;
                }
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
            case OP_LEAVE: // special because of compilation of loops
            case OP_JUMP:
                ip+=ip->d.i;
                break;
            case OP_IFLEAVE:
                if(popInt()){
                    ip+=ip->d.i;
                } else
                    ip++;
                break;
            case OP_DECLEAVENEG:{
                a = stack.peekptr();
                int q = a->toInt();
                q--;
                Types::tInteger->set(a,q);
                if(q<0){
                    stack.popptr();
                    ip += ip->d.i;
                } else
                    ip++;
                break;
            }
            case OP_ITERSTART:{
                a = stack.popptr(); // the iterable object
                // we make an iterator and push it onto the iterator stack
                b = loopIterStack.pushptr();
                a->t->createValueIterator(b,a);
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
            case OP_DOT:
                a = popval();
                puts(a->toString(strbuf1,1024));
                ip++;
                break;
            case OP_NEWLIST:
                Types::tList->set(pushval());
                ip++;
                break;
            case OP_CLOSELIST:
            case OP_APPENDLIST:
                a = popval();
                b = stack.peekptr(0);
                b = Types::tList->get(b)->append();
                b->copy(a);
                ip++;
                break;
            default:
                throw RUNT("unknown opcode");
            }
            
        }
    } catch(Exception e){
        // destroy any iterators left lying around
        while(!loopIterStack.isempty())
            loopIterStack.popptr()->clr();
        throw e;
    }
}


void Angort::define(const char *name,CompileContext *c){
    int idx;
    if((idx = words.get(name))<0)
        idx = words.add(name);
    Value *wordVal = words.get(idx);
    
    CodeBlock *cb = new CodeBlock(c);
    cb->spec = c->getSpec() ? strdup(c->getSpec()) : NULL;
    
    Types::tCode->set(wordVal,cb);
}
void Angort::define(const char *name,Instruction *i){
    int idx;
    if((idx = words.get(name))<0)
        idx = words.add(name);
    Value *wordVal = words.get(idx);
    
    CodeBlock *cb = new CodeBlock(i);
    
    Types::tCode->set(wordVal,cb);
}

void Angort::compileParamsAndLocals(){
    
    if(!defining && !inSubContext())
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
    FILE *ff = fopen(name,"r");
    lineNumber=0;
    try{
        char buf[1024];
        if(!ff)
            throw Exception().set("cannot open %s",name);
        while(fgets(buf,1024,ff)!=NULL){
            feed(buf);
            lineNumber++;
        }
        fclose(ff);
    }catch(Exception e){
        if(rethrow) throw e;
        printf("Error in file %s: %s\n",name,e.what());
        printf("Last line: %s\n",getLastLine());
        return false;
    } 
    return true;
}

const Instruction *Angort::compile(const char *s){
    // trick the system into thinking we're defining a colon
    // word for one line only
    defining = true;
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
}

void Angort::feed(const char *buf){
    if(printLines)
        printf(">>> %s\n",buf);
    
    if(!defining && context && !inSubContext()) // make sure we're reset unless we're compiling or subcontexting
        context->reset(NULL);
    
    strcpy(lastLine,buf);
    tok.reset(buf);
    int t,here;
    Instruction *code;
    try {
        for(;;){
            
            int t = tok.getnext();
            switch(t){
            case T_CONST: // const syntax = <val> const <ident>
                {
                    if(defining)
                        throw SyntaxException("'const' not allowed in a definition");
                    if(tok.getnext()!=T_IDENT)
                        throw SyntaxException("expected an identifier");
                    
                    if(consts.get(tok.getstring())>=0)
                        throw Exception().set("const %s already set",tok.getstring());
                    
                    int n = consts.add(tok.getstring());
                    // we write an instruction to 
                    // store this const
                    compile(OP_CONSTSET)->d.i=n;
                }
                break;
            case T_COLON:
                if(defining){
                    char spec[1024];
                    if(!tok.getnextstring(spec))
                        throw SyntaxException("expected spec string after second ':' in definition");
                    context->setSpec(spec);
                } else {             
                    defining = true;
                    if(!tok.getnextident(defineName))
                        throw SyntaxException("expected a word name");
                    if(tok.getnext()==T_COLON){
                    } else tok.rewind();
                }
                break;
            case T_DOT:
                compile(OP_DOT);
                break;
            case T_DEFER:
                if(defining)
                    throw SyntaxException("'defer' not allowed in a definition");
                if(!tok.getnextident(defineName))
                    throw SyntaxException("expected a word name");
                define(defineName,(Instruction *)NULL);
                break;
            case T_SEMI:
                if(!defining)
                    throw SyntaxException("; not allowed outside a definition");
                compile(OP_END);
                define(defineName,context);
                //                printf("defined %s - %d ops\n",defineName,context->getCodeSize());
                context->reset(NULL);
                defining = false;
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
            case T_TIMES:
                if(tok.getnext()!=T_OCURLY)
                    throw SyntaxException("times must be followed by {");
                // stack the value
                context->pushhere(); // this is our loop point
                context->pushleave(); // and which loop we're in
                context->compileAndAddToLeaveList(OP_DECLEAVENEG); // decrement and leave if neg
                break;
            case T_EACH:
                if(tok.getnext()!=T_OCURLY)
                    throw SyntaxException("each must be followed by {");
                context->compile(OP_ITERSTART); // creates and starts the iterator
                context->pushhere(); // loop point
                context->pushleave(); // and which loop we're in
                context->compileAndAddToLeaveList(OP_ITERLEAVEIFDONE);
                break;
            case T_OCURLY://start loop
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
                    if((t = consts.get(s))>=0){
                        compile(OP_CONSTGET)->d.i = t;
                    }  else if(NativeFunc f = funcs.get(s)){
                        compile(OP_FUNC)->d.func = f;
                    } else if((t = words.get(s))>=0){
                        compile(OP_WORD)->d.i = t;
                    } else
                        if(BAREWORDS){
                            char *s = strdup(tok.getstring());
                            compile(OP_LITERALSTRING)->d.s = s;
                        } else
                            throw SyntaxException(NULL)
                          .set("unknown identifier: %s",s);
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
                    if(tok.getnext()!=T_IDENT)
                        throw SyntaxException("expected an identifier");
                    else if((t = context->getLocalToken(tok.getstring()))>=0){
                        // if we couldn't find it as a local, try to find it as a 
                        // local in the parent context. If that succeeds, we will have
                        // created a local for it.
                        compile(OP_LOCALGET)->d.i=t;
                    } else if((t = context->findOrAttemptCreateClosure(tok.getstring()))>=0){
                        // we managed to create a closure from here to upstairs,
                        // so store the closure index
                        compile(OP_CLOSUREGET)->d.i=t;
                    } else if(Property *p = props.get(tok.getstring())){
                        compile(OP_PROPGET)->d.prop = p;
                    } else if((t = globals.get(tok.getstring()))>=0){
                        // it's a global; use it
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
                        if(tok.getcurrent()==T_EQUALS)
                            compile(OP_NEQUALS);
                        else
                            throw SyntaxException("expected an identifier");
                    }else if((t = context->getLocalToken(tok.getstring()))>=0){
                        // if we couldn't find it as a local, try to find it as a 
                        // local in the parent context. If that succeeds, we will have
                        // created a local for it.
                        compile(OP_LOCALSET)->d.i=t;
                    } else if((t = context->findOrAttemptCreateClosure(tok.getstring()))>=0){
                        // we managed to create a closure from here to upstairs,
                        // so store the closure index
                        compile(OP_CLOSURESET)->d.i=t;
                    } else if(Property *p = props.get(tok.getstring())){
                        compile(OP_PROPSET)->d.prop = p;
                    } else if((t = globals.get(tok.getstring()))>=0){
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
                
                if(globals.get(tok.getstring())<0)// ignore multiple defines
                    globals.add(tok.getstring());
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
                    lambdaContext->reset(NULL);
                }
                break;
            case T_END:
                // just return if we're still defining.
                if(!defining && !inSubContext()){
                    // otherwise run the buffer we just made
                    compile(OP_END);
                    run(context->getCode());
                    context->reset(NULL);
                }
                return;// and quit
            case T_PIPE:
                compileParamsAndLocals();
                break;
            case T_BACKTICK: // quote angort word
                {
                    if(tok.getnext()!=T_IDENT)
                        throw SyntaxException(NULL)
                          .set("expected an identifier, got %s",tok.getstring());
                    
                    t = words.get(tok.getstring());
                    if(t<0){
                        if(funcs.get(tok.getstring()))
                            throw SyntaxException(NULL).set("native word not applicable after `: %s",tok.getstring());
                        else
                            throw SyntaxException(NULL).set("undefined word: %s",tok.getstring());
                    }
                    compile(OP_LITERALCODE)->d.cb = words.get(t)->v.cb;
                }
                break;
                
            case T_OSQB: // create a new list
                compile(OP_NEWLIST);
                // if the next token is a close, just swallow it.
                if(tok.getnext()!=T_CSQB)
                    tok.rewind();
                break;
            case T_CSQB:
            case T_COMMA:
                compile(OP_APPENDLIST);
                break;
            default:
                throw SyntaxException(NULL).set("unhandled token: %s",
                                                tok.getstring());
            }
        }
    } catch(Exception e){
        // make sure we tidy up any state
        contextStack.clear(); // clear the context stack
        context = contextStack.pushptr();
        context->reset(NULL); // reset the old context
        defining=false;
        // make sure the return stack gets cleared otherwise
        // really strange things can happen on the next processed
        // line
        rstack.clear(); 
        closureStack.clear();
        throw e; // and rethrow
    }
}

void Angort::disasm(const char *name){
    int idx = words.get(name);
    if(idx<0)
        throw RUNT("unknown function");
    const CodeBlock *cb = words.get(idx)->v.cb;
    const Instruction *ip = cb->ip;
    const Instruction *base = ip;
    for(;;){
        int opcode = ip->opcode;
        showop(ip++,base);
        printf("\n");
        if(opcode == OP_END)break;
    }
}

const char *Angort::getSpec(const char *s){
    int idx;
    const char *spec;
    
    if((idx=words.get(s))>=0){
        Value *v = words.get(idx);
        return v->v.cb->spec;
    } else if(spec=funcSpecs.get(s)){
        return spec;
    } else if(spec=propSpecs.get(s)){
        return spec;
    }
    
    return NULL;
}

void Angort::list(){
    printf("GLOBALS:\n");
    globals.list();
    printf("CONSTANTS:\n");
    consts.list();
    printf("WORDS:\n");
    words.list();
    
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


void Angort::visitGlobalData(ValueVisitor *visitor){
    globals.visit(visitor);
    consts.visit(visitor);
    words.visit(visitor);
}




Closure::Closure(const CodeBlock *c,int tabsize,Value *t) : GarbageCollected() {
    if(c)
        Types::tCode->set(&codeBlockValue,c);
    ct=tabsize;
    table=t;
}
Closure::~Closure(){
    //    printf("closure deletion\n");
    delete [] table; // should delete AND DECREF the contained objects
}

Closure::Closure(const Closure *c) : GarbageCollected() {
    throw RUNT("CLOSURE COPY DISALLOWED");
    codeBlockValue.copy(&c->codeBlockValue);
    ct = c->ct;
    table = new Value[ct];
    for(int i=0;i<ct;i++)
        table[i].copy(c->table+i); // will INCREF the objects
}
