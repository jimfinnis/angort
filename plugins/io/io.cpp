/**
 * @file io.cpp
 * @brief  Brief description of file.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>

#include <angort/angort.h>
#include <angort/hash.h>

using namespace angort;

%name io
%shared


class File : public GarbageCollected {
public:
    File(FILE *_f){
        f = _f;
    }
    
    void close(){
        fclose(f);
    }
    
    
    ~File(){
        close();
    }
    FILE *f;
};

class FileType : public GCType {
public:
    FileType(){
        add("file","FILE");
    }
    
    FILE *get(Value *v){
        if(v->t!=this)
            throw RUNT("not a file");
        return ((File *)(v->v.gc))->f;
    }
    
    void set(Value *v,FILE *f){
        v->clr();
        v->t=this;
        v->v.gc = new File(f);
        incRef(v);
    }
};

static FileType tFile;

%word open 2 (path mode -- fileobj) open a file, modes same as fopen()
{
    Value *p[2];
    a->popParams(p,"ss");
    FILE *f = fopen(p[0]->toString().get(),p[1]->toString().get());
    if(!f)
        a->pushNone();
    else
        tFile.set(a->pushval(),f);
}

// possibly recursive binary write
static void dowrite(FILE *f,Value *v,bool inContainer=false){
    if(v->t == Types::tInteger){
        int32_t i = (int32_t)v->toInt();
        fwrite(&i,sizeof(i),1,f);
    } else if(v->t == Types::tFloat) {
        float i = (float)v->toFloat();
        fwrite(&i,sizeof(i),1,f);
    } else if(v->t == Types::tString || v->t == Types::tSymbol) {
        const StringBuffer &sb = v->toString();
        int len = strlen(sb.get());
        if(inContainer)len++; // in containers, append a NULL
        fwrite(sb.get(),len,1,f);
    } else if(v->t == Types::tList) {
        ArrayList<Value> *list = Types::tList->get(v);
        int32_t n = list->count();
        fwrite(&n,sizeof(n),1,f);
        
        ArrayListIterator<Value> iter(list);
        for(iter.first();!iter.isDone();iter.next()){
            Value *vv = iter.current();
            fwrite(&vv->t->id,sizeof(vv->t->id),1,f);
            dowrite(f,vv,true);
        }
    } else if(v->t == Types::tHash) {
        Hash *h = Types::tHash->get(v);
        int32_t n = h->count();
        fwrite(&n,sizeof(n),1,f);
        
        HashKeyIterator iter(h);
        for(iter.first();!iter.isDone();iter.next()){
            Value *vk = iter.current();
            fwrite(&vk->t->id,sizeof(vk->t->id),1,f);
            dowrite(f,vk,true);
            
            if(h->find(vk)){
                Value *vv = h->getval();
                fwrite(&vv->t->id,sizeof(vv->t->id),1,f);
                dowrite(f,vv,true);
            } else
                throw RUNT("unable to find value for key when saving hash");
        }
    } else {
        throw RUNT("").set("file write of unsupported type '%s'",v->t->name);
    }
}


static FILE *getf(Value *p,bool out){
    if(p->isNone())
        return out?stdout:stdin;
    else
        return tFile.get(p);
}

%word write (value fileobj/none --) write value as binary (int/float is 32 bits) to file or stdout
{
    Value *p[2];
    a->popParams(p,"vA",&tFile);
    
    dowrite(getf(p[1],true),p[0]);
}

%word write8 2 (value fileobj/none --) write signed byte
{
    Value *p[2];
    a->popParams(p,"nA",&tFile);
    int8_t b = p[0]->toInt();
    fwrite(&b,sizeof(b),1,getf(p[1],true));
}

%word write16 2 (value fileobj/none --) write 16-bit signed integer
{
    Value *p[2];
    a->popParams(p,"nA",&tFile);
    int16_t b = p[0]->toInt();
    fwrite(&b,sizeof(b),1,getf(p[1],true));
}    

%word write32 2 (value fileobj/none --) write 32-bit signed integer
{
    Value *p[2];
    a->popParams(p,"nA",&tFile);
    int32_t b = p[0]->toInt();
    fwrite(&b,sizeof(b),1,getf(p[1],true));
}    

%word writeu8 2 (value fileobj/none --) write unsigned byte
{
    Value *p[2];
    a->popParams(p,"nA",&tFile);
    uint8_t b = p[0]->toInt();
    fwrite(&b,sizeof(b),1,getf(p[1],true));
}

%word writeu16 2 (value fileobj/none --) write 16-bit unsigned integer
{
    Value *p[2];
    a->popParams(p,"nA",&tFile);
    uint16_t b = p[0]->toInt();
    fwrite(&b,sizeof(b),1,getf(p[1],true));
}    

%word writeu32 2 (value fileobj/none --) write 32-bit unsigned integer
{
    Value *p[2];
    a->popParams(p,"nA",&tFile);
    uint32_t b = p[0]->toInt();
    fwrite(&b,sizeof(b),1,getf(p[1],true));
}    

%word writefloat 2 (value fileobj/none --) write 32-bit float
{
    Value *p[2];
    a->popParams(p,"nA",&tFile);
    float b = p[0]->toFloat();
    fwrite(&b,sizeof(b),1,getf(p[1],true));
}    

%word readfloat 1 (fileobj/none -- float/none) read 32-bit float
{
    Value *p;
    float i;
    a->popParams(&p,"A",&tFile);
    
    if(fread(&i,sizeof(i),1,getf(p,false))>0)
        a->pushFloat(i);
    else
        a->pushNone();
}

%word read8 1 (fileobj/none -- int/none) read signed byte
{
    Value *p;
    int8_t i;
    a->popParams(&p,"A",&tFile);
    
    if(fread(&i,sizeof(i),1,getf(p,false))>0)
        a->pushInt((int)i);
    else
        a->pushNone();
}
%word read16 1 (fileobj/none -- int/none) read 16-bit signed int
{
    Value *p;
    int16_t i;
    a->popParams(&p,"A",&tFile);
    
    if(fread(&i,sizeof(i),1,getf(p,false))>0)
        a->pushInt((int)i);
    else
        a->pushNone();
}
%word read32 1 (fileobj/none -- int/none) read 32-bit signed int
{
    Value *p;
    int16_t i;
    a->popParams(&p,"A",&tFile);
    
    if(fread(&i,sizeof(i),1,getf(p,false))>0)
        a->pushInt((int)i);
    else
        a->pushNone();
}

%word readu8 1 (fileobj/none -- int/none) read unsigned byte
{
    Value *p;
    uint8_t i;
    a->popParams(&p,"A",&tFile);
    
    if(fread(&i,sizeof(i),1,getf(p,false))>0)
        a->pushInt((int)i);
    else
        a->pushNone();
}
%word readu16 1 (fileobj/none -- int/none) read 16-bit unsigned int
{
    Value *p;
    uint16_t i;
    a->popParams(&p,"A",&tFile);
    
    if(fread(&i,sizeof(i),1,getf(p,false))>0)
        a->pushInt((int)i);
    else
        a->pushNone();
}
%word readu32 1 (fileobj/none -- int/none) read 32-bit unsigned int
{
    Value *p;
    uint32_t i;
    a->popParams(&p,"A",&tFile);
    
    if(fread(&i,sizeof(i),1,getf(p,false))>0)
        a->pushInt((int)i);
    else
        a->pushNone();
}


/// allocates a data buffer!
static const char *readstr(FILE *f,bool endAtEOL=false){
    int bufsize = 128;
    int ct=0;
    char *buf = (char *)malloc(bufsize);
    
    for(;;){
        char c = fgetc(f);
        if(c==EOF || (endAtEOL && (c=='\n' || c=='\r')) || !c)
            break;
        if(ct==bufsize){
            bufsize *= 2;
            buf = (char *)realloc(buf,bufsize);
        }
        buf[ct++]=c;
    }
    buf[ct]=0;
    return buf;
}

%word readstr 1 (fileobj/none -- str) read string until null/EOL/EOF
{
    Value *p;
    a->popParams(&p,"A",&tFile);
    
    FILE *f = getf(p,false);
    const char *s = readstr(f);
    a->pushString(s);
    free((char *)s);
}

%word readfilestr 1 (fileobj/none -- str) read an entire text file
{
    Value *p;
    a->popParams(&p,"A",&tFile);
    
    FILE *f = getf(p,false);
    const char *s = readstr(f,false);
    a->pushString(s);
    free((char *)s);
}

%word eof 1 (fileobj/none -- boolean) indicates if EOF has been read
{
    Value *p;
    a->popParams(&p,"A",&tFile);
    
    FILE *f = getf(p,false);
    a->pushInt(feof(f)?1:0);
}
    
static void doreadlist(FILE *f,Value *res);
static void doreadhash(FILE *f,Value *res);

static bool readval(FILE *f,Value *res){
    
    uint32_t typeID;
    int32_t i;
    float fl;
    
    if(fread(&typeID,sizeof(typeID),1,f)<=0)
        return false;
    
    if(typeID == Types::tInteger->id) {
        fread(&i,sizeof(i),1,f);
        Types::tInteger->set(res,(int)i);
    } else if(typeID == Types::tFloat->id) {
        fread(&fl,sizeof(fl),1,f);
        Types::tFloat->set(res,fl);
    } else if(typeID == Types::tString->id) {
        const char *s = readstr(f);
        Types::tString->set(res,s);
        free((char *)s);
    } else if(typeID == Types::tSymbol->id) {
        const char *s = readstr(f);
        int sid = SymbolType::getSymbol(s);
        Types::tSymbol->set(res,sid);
        free((char *)s);
    } else if(typeID == Types::tList->id) {
        doreadlist(f,res);
    } else if(typeID == Types::tHash->id) {
        doreadhash(f,res);
    } else
        throw RUNT("").set("file read of unsupported type %x",typeID);
    return true;
}
    
static void doreadlist(FILE *f,Value *res){
    ArrayList<Value> *list = Types::tList->set(res);
    int32_t n;
    fread(&n,sizeof(n),1,f);
    for(int i=0;i<n;i++){
        Value *v = list->append();
        if(!readval(f,v))
            throw RUNT("").set("premature end of list read");
    }
}
static void doreadhash(FILE *f,Value *res){
    Hash *h = Types::tHash->set(res);
    
    int32_t n;
    fread(&n,sizeof(n),1,f);
    try {
        for(int i=0;i<n;i++){
            Value k,v;
            if(!readval(f,&k))
                throw "key";
            if(!readval(f,&v))
                throw "val";
            h->set(&k,&v);
        }
    } catch(const char *s) {
        throw RUNT("").set("badly formed hash in read on reading %s",s);
    }
}

%word readlist 1 (fileobj/none -- list) read a binary list (as written by 'write')
{
    Value *p;
    a->popParams(&p,"A",&tFile);
    FILE *f = getf(p,false);
    doreadlist(f,a->pushval());
}
%word readhash 1 (fileobj/none -- hash) read a binary hash (as written by 'write')
{
    Value *p;
    a->popParams(&p,"A",&tFile);
    FILE *f = getf(p,false);
    doreadhash(f,a->pushval());
}

%word exists 1 (path -- boolean/none) does a file/directory exist? None indicates some other problem
{
    Value *p;
    a->popParams(&p,"s");
    
    struct stat b;
    if(stat(p->toString().get(),&b)==0)
        a->pushInt(1);
    else if(errno==ENOENT)
        a->pushInt(0);
    else
        a->pushNone();
}

%word flush 1 (fileobj/none -- ) flush the file buffer
{
    Value *p;
    a->popParams(&p,"A",&tFile);
    FILE *f = getf(p,true);
    fflush(f);
}

inline void setIntInHash(Hash *h, const char *key,uint32_t value){
    Value k,v;
    Types::tString->set(&k,key);
    Types::tInteger->set(&v,(int)value);
    h->set(&k,&v);
}

%word stat 1 (path -- hash/none) read the file statistics, or none if not found
{
    Value *p;
    a->popParams(&p,"s");
    
    struct stat b;
    Value *res = a->pushval();
    
    if(stat(p->toString().get(),&b)==0){
        Hash *h = Types::tHash->set(res);
        
        setIntInHash(h,"mode",b.st_mode);
        setIntInHash(h,"uid",b.st_uid);
        setIntInHash(h,"gid",b.st_gid);
        setIntInHash(h,"size",b.st_size);
        setIntInHash(h,"atime",b.st_atime);
        setIntInHash(h,"mtime",b.st_mtime);
        setIntInHash(h,"ctime",b.st_ctime);
    } else
        res->clr();
}

%init
{
    printf("Initialising IO plugin, %s %s\n",__DATE__,__TIME__);
}
