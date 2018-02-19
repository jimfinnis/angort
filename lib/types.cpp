#include "angort.h"
#include "cycle.h"

namespace angort {

Type *Type::head=NULL;
static bool createTypesDone=false;

int  GarbageCollected::globalCount=0;

//#define tdprintf printf
#define tdprintf if(0)printf

const char *Type::toString(bool *allocated,const Value *v) const{
    char buf[128];
    snprintf(buf,128,"<TYPE %s:%p>",name,v->v.s);
    *allocated=true;
    return strdup(buf);
}

float Type::toFloat(const Value *v) const{
    throw BadConversionException(v->t->name,Types::tFloat->name);
}

int Type::toInt(const Value *v) const {
    throw BadConversionException(v->t->name,Types::tInteger->name);
}
long Type::toLong(const Value *v) const {
    throw BadConversionException(v->t->name,Types::tLong->name);
}
double Type::toDouble(const Value *v) const {
    throw BadConversionException(v->t->name,Types::tDouble->name);
}

void Type::toSelf(Value *out,const Value *v) const {
    throw BadConversionException(v->t->name,name);
}

void Type::makeNumber(){
    flags |= TF_NUMBER;
    supertype = Types::tNumber;
}
void Type::makeStringable(){
    supertype = Types::tStringStrict;
}


void Type::createIterator(Value *dest,Value *src)const{
    Iterator<Value *> *i = makeIterator(src);
    i->first();
    Types::tIter->set(dest,src,i);
}

void Type::clone(Value *out,const Value *in,bool deep)const{
    // default action is to just copy the value; collections
    // need to do more.
    out->copy(in);
}

void Type::add(const char *_name,const char *_id){
    if(getByName(_name))
        throw Exception(EX_DEFINED).set("type already exists: %s",name);
    
    const unsigned char *n = (const unsigned char *)_id;
    id = n[0]+(n[1]<<8)+(n[2]<<16)+(n[3]<<24);
    name = _name;
    next = head;
    head = this;
    
    // create the binop ID
    static int binopIDCount=0;
    binopID = binopIDCount++;
    
    // normally symbols are generated after the static initialisation
    // but before anything else, in createTypes(). This is to make sure
    // the symbol system has been initialised.
    // If this type is being added later, say by a plugin, we need to
    // do it here.
    
    if(createTypesDone){
        nameSymb = SymbolType::getSymbol(name);
    }
}

Type *Type::getByID(const char *_id){
    const unsigned char *n = (const unsigned char *)_id;
    uint32_t i = n[0]+(n[1]<<8)+(n[2]<<16)+(n[3]<<24);
    
    for(Type *t = head;t;t=t->next)
        if(t->id == i)return t;
    return NULL;
}


void Type::registerBinop(Type *rhs, int opcode, BinopFunction f){
    uint32_t key = (rhs->binopID << 16) + opcode;
    BinopFunction *ptr = binops.set(key);
    *ptr = f;
}

bool Type::binop(Runtime *a,int opcode,Value *lhs,Value *rhs){
    // have to cast away the constness because getBinop can't be const.
    BinopFunction *f = const_cast<Type *>(lhs->t)->getBinop(rhs->t,opcode);
    if(!f)
        return false;
    (**f)(a,lhs,rhs);
    return true;
}


char *BlockAllocType::allocate(Value *v,int len,const Type *type)const{
    v->clr();
    BlockAllocHeader *h = (BlockAllocHeader *)malloc(len+sizeof(BlockAllocHeader));
    h->refct=1;
    v->v.block = h;
    v->t = type;
    return (char *)(h+1);
}

int Type::getIndexOfContainedItem(Value *v,Value *item)const{
    Iterator<Value *> *iter = makeIterator(v); // will throw for non-iterables
    int i=0;
    for(iter->first();!iter->isDone();iter->next()){
        if(iter->current()->equalForHashTable(item)){
            delete iter;
            return i;
        }
        i++;
    }
    delete iter;
    return -1;
}

const char *BlockAllocType::getData(const Value *v) const{
    return (const char *)(v->v.block+1);
}

void BlockAllocType::incRef(Value *v)const{
    WriteLock lock=WL(&globalLock);
    BlockAllocHeader *h = v->v.block;
    h->refct++;
    tdprintf("INCREF STR to %d: %p%s\n",h->refct,getData(v),getData(v));
    if(!h->refct)
        h->refct=0xffff; // MAX REFCOUNT is never freed!
    //        throw RUNT("reference count too large");
}
    
void BlockAllocType::decRef(Value *v)const{
    WriteLock lock=WL(&globalLock);
    BlockAllocHeader *h = v->v.block;
    if(h->refct!=0xffff)h->refct--; // MAX REFCOUNT is never freed!
    tdprintf("DECREF STR to %d: %p%s\n",h->refct,getData(v),getData(v));
    if(h->refct==0){
        free(h);
    }
}

void GCType::incRef(Value *v)const{
    WriteLock lock=WL(&globalLock);
    v->v.gc->incRefCt();
    tdprintf("incrementing ref count of %s:%p, now %d\n",name,v->v.gc,v->v.gc->refct);
}

void GCType::decRef(Value *v)const{
    WriteLock lock=WL(&globalLock);
    bool b = v->v.gc->decRefCt();
    tdprintf("decrementing ref count of %s:%p, now %d\n",name,v->v.gc,v->v.gc->refct);
    if(b){
        tdprintf("  AND DELETING %s:%p\n",name,v->v.gc);
        delete v->v.gc;
    }
}

Iterator<class Value *> *GCType::makeKeyIterator(Value *v)const{
    return v->v.gc->makeKeyIterator();
}
Iterator<class Value *> *GCType::makeValueIterator(Value *v)const{
    return v->v.gc->makeValueIterator();
}
    

GarbageCollected *GCType::getGC(Value *v)const{
    return v->v.gc;
}

GarbageCollected::GarbageCollected() : Lockable("gc"){
    WriteLock lock=WL(&globalLock);
    refct=0;
    globalCount++;
    CycleDetector::getInstance()->add(this);
    
}



GarbageCollected::~GarbageCollected(){
    WriteLock lock=WL(&globalLock);
    globalCount--;
    CycleDetector::getInstance()->remove(this);
    refct=-999; // to mark was deleted properly; snark.
}





/*
 * 
 * New types go down here
 * 
 */

static NoneType _tNone;
NoneType *Types::tNone= &_tNone;

// "pseudotypes" used to identify numbers and strings
static Type _tNumber;
Type *Types::tNumber = &_tNumber;
static Type _tStringStrict;
Type *Types::tStringStrict = &_tStringStrict;

static Type _tDeleted; // has to be added by hand in createTypes
Type *Types::tDeleted= &_tDeleted;

static IntegerType _Int;
IntegerType *Types::tInteger = &_Int;

static NSIDType _NSID;
NSIDType *Types::tNSID = &_NSID;

static FloatType _Float;
FloatType *Types::tFloat = &_Float;

static StringType _String;
StringType *Types::tString = &_String;

static CodeType _Code;
CodeType *Types::tCode = &_Code;

static ClosureType _Closure;
ClosureType *Types::tClosure = &_Closure;

static RangeType<int> _IRange;
RangeType<int> *Types::tIRange = &_IRange;
static RangeType<float> _FRange;
RangeType<float> *Types::tFRange = &_FRange;

static ListType _List;
ListType *Types::tList = &_List;

static HashType _Hash;
HashType *Types::tHash = &_Hash;

static SymbolType _Symbol;
SymbolType *Types::tSymbol = &_Symbol;

static NativeType _Native;
NativeType *Types::tNative = &_Native;

static PropType _Prop;
PropType *Types::tProp = &_Prop;

static LongType _Long;
LongType *Types::tLong = &_Long;

static DoubleType _Double;
DoubleType *Types::tDouble = &_Double;



static IteratorType _Iterator;
IteratorType *Types::tIter = &_Iterator;



void Types::createTypes(){
    tDeleted->add("DELETED","DELE");
    tStringStrict->add("stringstrict","STXX");
    tNumber->add("numeric","NUMB");
    
    // because of the undefined execution order of static heap
    // objects, we set up all the type names here rather than in
    // "add" to make sure the symbol system is already up.
    
    // IF you add types later in plugins, the system should detect
    // this.
    
    for(Type *p = Type::head;p;p=p->next){
        p->nameSymb = SymbolType::getSymbol(p->name);
    }
    createTypesDone = true;
}
}
