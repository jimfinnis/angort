/**
 * @file binop.cpp
 * @brief  Binary operations are handled in this file, along with the appropriate coercions.
 *
 */

#include "angort.h"
#include "hash.h"
#include "opcodes.h"

namespace angort {

/// append to a list. If the element to add is itself a list,
/// split it apart and and the items individually.

static void addListOrValueToList(ArrayList<Value> *list,Value *a){
    if(a->getType() == Types::tList){
        ArrayListIterator<Value> iter(Types::tList->get(a));
        
        for(iter.first();!iter.isDone();iter.next()){
            list->append()->copy(iter.current());
        }
    } else 
        list->append()->copy(a);
}    

static void addHashToHash(Hash *h,Hash *h2){
    HashKeyIterator iter(h2);
    for(iter.first();!iter.isDone();iter.next()){
        Value *k = iter.current();
        Value *v;
        if(h2->find(k))
            v = h2->getval();
        else
            throw WTF;
        
        h->set(k,v);
    }
}

/**
 * Take a pair of values a,b and perform the binary operation given.
 * The result should be pushed onto the stack. The instruction pointer
 * is incremented by the caller.
 */

void Angort::binop(Value *a,Value *b,int opcode){
    
    Type *at = a->getType();
    Type *bt = b->getType();
    
    // first look to see if it's a registered binop. If
    // so, run it. Otherwise fall through to a set of if
    // statements.
    
    if(at->binop(this,opcode,a,b))
        return;
    
    
    if(at == Types::tNone ||  bt == Types::tNone){
        /**
         * 
         * One of the values is NONE
         *
         */
        switch(opcode){
        case OP_EQUALS: // none is equal to nothing
            pushInt(0);break;
        case OP_NEQUALS:// and not equal to everything
            pushInt(1);break;
        default:
            throw RUNT("invalid operation with a 'none' operand");
        }
    }else if(at == Types::tList || bt == Types::tList) {
        ListObject *lo;
        switch(opcode){
            case OP_ADD:
            lo = new ListObject();
            addListOrValueToList(&lo->list,a);
            addListOrValueToList(&lo->list,b);
            break;
        default:
            throw RUNT("invalid operation with a list operand");
        }
        Types::tList->set(pushval(),lo);
    }else if(at == Types::tHash || bt == Types::tHash) {
        HashObject *ho;
        switch(opcode){
            case OP_ADD:
            ho = new HashObject();
            addHashToHash(ho->hash,a->v.hash->hash);
            addHashToHash(ho->hash,b->v.hash->hash);
            break;
        default:
            throw RUNT("invalid operation with a list operand");
        }
        Types::tHash->set(pushval(),ho);
    } else if((at == Types::tString || at == Types::tSymbol) &&
              bt == Types::tInteger &&
              opcode == OP_MUL){
        /**
         * Special case for multiplying a string/symbol by a number
         */
        const StringBuffer& p = a->toString();
        
        int reps = b->toInt();
        int len = strlen(p.get());
        Value t; // temp value
        // allocate a new result
        char *q = Types::tString->allocate(&t,len*reps+1,Types::tString);
        for(int i=0;i<reps;i++){
            memcpy(q+i*len,p.get(),len);
        }
        q[len*reps]=0;
        stack.pushptr()->copy(&t);
    } else if(at == Types::tString || bt == Types::tString){
        /**
         * 
         * One of the value is a string; coerce the other to a string and then
         * perform a string operation.
         *
         */
        const StringBuffer& p = a->toString();
        const StringBuffer& q = b->toString();
        switch(opcode){
        case OP_ADD:{
            Value t; // we use a temp, otherwise allocate() will clear the type
            int len = strlen(p.get())+strlen(q.get());
            char *r = Types::tString->allocate(&t,len+1,Types::tString);
            strcpy(r,p.get());
            strcat(r,q.get());
            stack.pushptr()->copy(&t);
            break;
        }
        case OP_EQUALS:
            pushInt(!strcmp(p.get(),q.get()));
            break;
        case OP_NEQUALS:
            pushInt(strcmp(p.get(),q.get()));
            break;
        case OP_GT:
            pushInt(strcmp(p.get(),q.get())>0);
            break;
        case OP_LT:
            pushInt(strcmp(p.get(),q.get())<0);
            break;
        case OP_GE:
            pushInt(strcmp(p.get(),q.get())>=0);
            break;
        case OP_LE:
            pushInt(strcmp(p.get(),q.get())<=0);
            break;
        case OP_CMP:
            pushInt(strcmp(p.get(),q.get()));
            break;
        default:throw RUNT("bad operation for strings");
        }
        
    }else if(at == Types::tFloat || bt == Types::tFloat){
        /**
         * One of the values is a float; coerce the other to a float and perform
         * a float operation
         */
        float r;
        float p = a->toFloat();
        float q = b->toFloat();
        bool cmp=false;
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
        case OP_GE:
            cmp=true;
            pushInt(p>=q);break;
        case OP_LE:
            cmp=true;
            pushInt(p<=q);break;
        case OP_CMP:
            cmp=true;
            pushInt(((p-q)>0)?1:(((p-q)<0)?-1:0));
            break;
        default:
            throw RUNT("invalid operator for floats");
        }
        if(!cmp)
            pushFloat(r);
    }else if(at == Types::tSymbol || bt == Types::tSymbol){
        /*
         * One of the values is a symbol; equality tests on the integers or
         * string comparisons are valid. Note that < and > work as expected,
         * and equality with strings will work because that will be dealt with
         * by the string test above
         */
        switch(opcode){
        case OP_EQUALS:
            pushInt((a->v.i == b->v.i)?1:0);break;
        case OP_NEQUALS:
            pushInt((a->v.i != b->v.i)?1:0);break;
        case OP_GT:{
            const StringBuffer& p = a->toString();
            const StringBuffer& q = b->toString();
            pushInt(strcmp(p.get(),q.get())>0);
            break;
        }
        case OP_LT:{
            const StringBuffer& p = a->toString();
            const StringBuffer& q = b->toString();
            pushInt(strcmp(p.get(),q.get())<0);
            break;
        }
        case OP_GE:{
            const StringBuffer& p = a->toString();
            const StringBuffer& q = b->toString();
            pushInt(strcmp(p.get(),q.get())>=0);
            break;
        }
        case OP_LE:{
            const StringBuffer& p = a->toString();
            const StringBuffer& q = b->toString();
            pushInt(strcmp(p.get(),q.get())<=0);
            break;
        }
        case OP_CMP:{
            const StringBuffer& p = a->toString();
            const StringBuffer& q = b->toString();
            pushInt(strcmp(p.get(),q.get()));
            break;
        }
        default:
            throw RUNT("bad operation for symbols");
        }
    } else {
        /**
         * Otherwise assume we're dealing with ints
         */
        
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
        case OP_GE:
            p = a->toInt();
            q = b->toInt();
            r = (p>=q);break;
        case OP_LE:
            p = a->toInt();
            q = b->toInt();
            r = (p<=q);break;
        case OP_CMP:
            p = a->toInt();
            q = b->toInt();
            r = ((p-q)>0)?1:(((p-q)<0)?-1:0);
            break;
        case OP_EQUALS:
            if(at == Types::tInteger || bt == Types::tInteger)
                r = a->toInt() == b->toInt();
            else
                r = (a->v.s == b->v.s);
            break;
        case OP_NEQUALS:
            if(at == Types::tInteger || bt == Types::tInteger)
                r = a->toInt() != b->toInt();
            else
                r = (a->v.s != b->v.s);
            break;
        default:
            throw WTF;
            
        }
        pushInt(r);
    }
    
}
}
