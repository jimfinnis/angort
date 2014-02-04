#include "angort.h"
#include "ser.h"
#include "file.h"

Type *Type::head = NULL;

const char *Type::toString(char *outBuf,int len,const Value *v) const{
    snprintf(outBuf,len,"<TYPE %s:?%x?>",name,v->v.i);
    return outBuf;
}

float Type::toFloat(const Value *v) const{
    throw BadConversionException(v->t->name,Types::tFloat->name);
}

int Type::toInt(const Value *v) const {
    throw BadConversionException(v->t->name,Types::tInteger->name);
}

void Type::createValueIterator(Value *dest,Value *src){
    Iterator<Value *> *i = makeValueIterator(src);
    i->first();
    Types::tIter->set(dest,i);
}

void Type::createKeyIterator(Value *dest,Value *src){
    Iterator<Value *> *i = makeKeyIterator(src);
    i->first();
    Types::tIter->set(dest,i);
}





char *BlockAllocType::allocate(Value *v,int len){
    BlockAllocHeader *h = (BlockAllocHeader *)malloc(len+sizeof(BlockAllocHeader));
    h->refct=1;
    v->v.block = h;
    return (char *)(h+1);
}


const char *BlockAllocType::getData(const Value *v) const{
    return (const char *)(v->v.block+1);
}

void BlockAllocType::incRef(Value *v){
    BlockAllocHeader *h = v->v.block;
    h->refct++;
    if(!h->refct)
        throw RUNT("reference count too large");
}
    
void BlockAllocType::decRef(Value *v){
    BlockAllocHeader *h = v->v.block;
    h->refct--;
    if(h->refct==0){
        free(h);
    }
}

void BlockAllocType::saveValue(Serialiser *ser, Value *v){
    ser->file->write32(ser->getFixupByData(v->v.cb));
}
void BlockAllocType::loadValue(Serialiser *ser, Value *v){
    v->setFixup(ser->file->read32());
}



void GCType::incRef(Value *v){
    v->v.gc->incRefCt();
//    printf("incrementing ref count of %s:%p, now %d\n",name,v->v.gc,v->v.gc->refct);
}
    
void GCType::decRef(Value *v){
    bool b = v->v.gc->decRefCt();
//    printf("decrementing ref count of %s:%p, now %d\n",name,v->v.gc,v->v.gc->refct);
    if(b){
        delete v->v.gc;
//        printf("  AND DELETING\n");
    }
}


void GCType::saveValue(Serialiser *ser, Value *v){
    ser->file->write32(ser->getFixupByData(v->v.gc));
}
void GCType::loadValue(Serialiser *ser, Value *v){
    v->setFixup(ser->file->read32());
}



/*
 * 
 * New types go down here
 * 
 */

static Type _tNone;
Type *Types::tNone= &_tNone;
static Type _tFixup;
Type *Types::tFixup= &_tFixup;

static IntegerType _Int;
IntegerType *Types::tInteger = &_Int;

static FloatType _Float;
FloatType *Types::tFloat = &_Float;

static StringType _String;
StringType *Types::tString = &_String;

static CodeType _Code;
CodeType *Types::tCode = &_Code;

static ClosureType _Closure;
ClosureType *Types::tClosure = &_Closure;

static RangeType _Range;
RangeType *Types::tRange = &_Range;

static ListType _List;
ListType *Types::tList = &_List;


static IteratorType _Iterator;
IteratorType *Types::tIter = &_Iterator;



void Types::createTypes(){
    tNone->add("NONE","NONE");
    tFixup->add("fixup","FIXP");
    
}
