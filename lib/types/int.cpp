/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */


#include "angort.h"

namespace angort {

    

int IntegerType::get(Value *v) const {
    /*
    if(v->t == Types::tInteger)
        return v->v.i;
    else if(v->t == Types::tFloat)
        return (int)v->v.f;
    else
       throw BadConversionException(v->t->name,name);
     */
    return v->t->toInt(v);
}


void IntegerType::set(Value *v,int i) const {
    v->clr();
    v->t = Types::tInteger;
    v->v.i=i;
}

int IntegerType::toInt(const Value *v) const {
    return v->v.i;
}

float IntegerType::toFloat(const Value *v) const {
    return (float)v->v.i;
}

double IntegerType::toDouble(const Value *v) const {
    return (double)v->v.i;
}

long IntegerType::toLong(const Value *v) const {
    return (long)v->v.i;
}

void IntegerType::toSelf(Value *out,const Value *v) const {
    set(out,v->toInt());
}


const char *IntegerType::toString(bool *allocated,const Value *v) const {
    char buf[128];
    snprintf(buf,128,"%d",v->v.i);
    *allocated=true;
    return strdup(buf);
}

uint32_t IntegerType::getHash(Value *v) const {
    return (uint32_t)v->v.i;
}

bool IntegerType::equalForHashTable(Value *a,Value *b) const {
    return a->toInt() == b->toInt();
}

void IntegerType::absolute(Value *dest,Value *src) const {
    int v = src->toInt();
    set(dest,v<0?-v:v);
}

void IntegerType::negate(Value *dest,Value *src) const {
    set(dest,-src->toInt());
}


void IntegerType::increment(Value *v,int step) const {
    v->v.i += step;
}


class IntegerIterator : public Iterator<Value *> {
private:
    Value val; // result
    int top;
public:
    IntegerIterator(const Value *v){
        top = v->v.i;
        Types::tInteger->set(&val,0);
    }
    virtual ~IntegerIterator(){}
    virtual void first(){
        val.v.i = 0;
    }
    virtual void next(){
        if(top<0)
            val.v.i--;
        else
            val.v.i++;
    }
    virtual bool isDone() const {
        if(top<0)
            return val.v.i <= top;
        else
            return val.v.i >= top;
    }
    virtual Value *current() {
        return &val;
    }
    virtual int index() const {
        return val.v.i;
    }
};


Iterator<Value *> *IntegerType::makeValueIterator(Value *v)const{
    return new IntegerIterator(v);
}


}
