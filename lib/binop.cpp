/**
 * @file binop.cpp
 * @brief  Binary operations are handled in this file, along with the appropriate coercions.
 *
 */

#include "angort.h"
#include "opcodes.h"

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

/**
 * Take a pair of values a,b and perform the binary operation given.
 * The result should be pushed onto the stack. The instruction pointer
 * is incremented by the caller.
 */

void Angort::binop(Value *a,Value *b,int opcode){
    
    Type *at = a->getType();
    Type *bt = b->getType();
    
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
    } else if((at == Types::tString || at == Types::tSymbol) && bt == Types::tInteger && opcode == OP_MUL){
        /**
         * Special case for multiplying a string/symbol by a number
         */
        const char * p = a->toString(strbuf1,1024);
        
        int reps = b->toInt();
        int len = strlen(p);
        Value t; // temp value
        // allocate a new result
        char *q = Types::tString->allocate(&t,len*reps+1,Types::tString);
        for(int i=0;i<reps;i++){
            memcpy(q+i*len,p,len);
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
        bool cmp=false;
        // buffers won't be used if they're already strings
        const char * p = a->toString(strbuf1,1024);
        const char * q = b->toString(strbuf2,1024);
        switch(opcode){
        case OP_ADD:{
            Value t; // we use a temp, otherwise allocate() will clear the type
            int len = strlen(p)+strlen(q);
            char *r = Types::tString->allocate(&t,len+1,Types::tString);
            strcpy(r,p);
            strcat(r,q);
            stack.pushptr()->copy(&t);
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
        
    }else if(at == Types::tFloat || bt == Types::tFloat){
        /**
         * One of the values is a float; coerce the other to a float and perform
         * a float operation
         */
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
    }else if(at == Types::tSymbol || bt == Types::tSymbol){
        /*
         * One of the values is a symbol; equality tests on the integers or
         * string comparisons are valid. Note that < and > work as expected,
         * and equality with strings will work because that will be dealt with
         * by the string test above
         */
        bool r;
        switch(opcode){
        case OP_EQUALS:
            r = a->v.i == b->v.i;break;
        case OP_NEQUALS:
            r = a->v.i != b->v.i;break;
        case OP_GT:{
            const char * p = a->toString(strbuf1,1024);
            const char * q = b->toString(strbuf2,1024);
            r = (strcmp(p,q)>0);
            break;
        }
        case OP_LT:{
            const char * p = a->toString(strbuf1,1024);
            const char * q = b->toString(strbuf2,1024);
            r = (strcmp(p,q)<0);
            break;
        }
        default:
            throw RUNT("bad operation for symbols");
        }
        pushInt(r?1:0);
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
            
        }
        pushInt(r);
    }
    
}
