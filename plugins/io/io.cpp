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

#include <angort/plugins.h>

%plugin io

class File : public PluginObject {
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

%word open 2 (path mode -- fileobj) open a file, modes same as fopen()
{
    FILE *f = fopen(params[0].getString(),params[1].getString());
    if(!f)
        res->setNone();
    else {
        res->setObject(new File(f));
    }
}

// possibly recursive binary write
static void dowrite(FILE *f,PluginValue *v,bool inContainer=false){
    switch(v->type){
    case PV_INT:{
        int32_t i = (int32_t)v->getInt();
        fwrite(&i,sizeof(i),1,f);
    }
        break;
    case PV_FLOAT:{
        float i = (float)v->getFloat();
        fwrite(&i,sizeof(i),1,f);
    }
        break;
    case PV_STRING:
    case PV_SYMBOL:{
        int len = strlen(v->getString());
        if(inContainer)len++; // in containers, append a NULL
        fwrite(v->getString(),len,1,f);
    }
        break;
    case PV_LIST:{
        int n = v->getListCount();
        fwrite(&n,sizeof(n),1,f);
        for(PluginValue *p=v->v.head;p;p=p->next){
            fwrite(&p->type,sizeof(p->type),1,f);
            dowrite(f,p,true);
        }
        break;
    }
    case PV_HASH:{
        int n = v->getHashCount();
        fwrite(&n,sizeof(n),1,f);
        for(PluginValue *p=v->v.head;p;p=p->next){
            fwrite(&p->type,sizeof(p->type),1,f);
            dowrite(f,p,true);
        }
        break;
    }
    default:
        throw "file write of unsupported type";
    }
}

static FILE *getf(PluginValue *p,bool out){
    if(p->type == PV_NONE)
        return out?stdout:stdin;
    else
        return ((File *)p->getObject())->f;
}

%word write 2 (value fileobj/none --) write value as binary (int/float is 32 bits) to file or stdout
{
    dowrite(getf(params+1,true),params);
}

%word write8 2 (value fileobj/none --) write signed byte
{
    int8_t b = params->getInt();
    fwrite(&b,sizeof(b),1,getf(params+1,true));
}

%word write16 2 (value fileobj/none --) write 16-bit signed integer
{
    int16_t b = params->getInt();
    fwrite(&b,sizeof(b),1,getf(params+1,true));
}    

%word write32 2 (value fileobj/none --) write 32-bit signed integer
{
    int16_t b = params->getInt();
    fwrite(&b,sizeof(b),1,getf(params+1,true));
}    

%word writeu8 2 (value fileobj/none --) write unsigned byte
{
    uint8_t b = params->getInt();
    fwrite(&b,sizeof(b),1,getf(params+1,true));
}

%word writeu16 2 (value fileobj/none --) write 16-bit unsigned integer
{
    uint16_t b = params->getInt();
    fwrite(&b,sizeof(b),1,getf(params+1,true));
}    

%word writeu32 2 (value fileobj/none --) write 32-bit unsigned integer
{
    uint16_t b = params->getInt();
    fwrite(&b,sizeof(b),1,getf(params+1,true));
}    

%word writefloat 2 (value fileobj/none --) write 32-bit float
{
    float b = params->getFloat();
    fwrite(&b,sizeof(b),1,getf(params+1,true));
}    

%word readfloat 1 (fileobj/none -- float/none) read 32-bit float
{
    float i;
    if(fread(&i,sizeof(i),1,getf(params,false))>0)
        res->setFloat(i);
    else
        res->setNone();
}

%word read8 1 (fileobj/none -- int/none) read signed byte
{
    int8_t i;
    if(fread(&i,sizeof(i),1,getf(params,false))>0)
        res->setInt((int)i);
    else
        res->setNone();
}
%word read16 1 (fileobj/none -- int/none) read 16-bit signed int
{
    int16_t i;
    if(fread(&i,sizeof(i),1,getf(params,false))>0)
        res->setInt((int)i);
    else
        res->setNone();
}
%word read32 1 (fileobj/none -- int/none) read 32-bit signed int
{
    int32_t i;
    if(fread(&i,sizeof(i),1,getf(params,false))>0)
        res->setInt((int)i);
    else
        res->setNone();
}

%word readu8 1 (fileobj/none -- int/none) read unsigned byte
{
    uint8_t i;
    if(fread(&i,sizeof(i),1,getf(params,false))>0)
        res->setInt((int)i);
    else
        res->setNone();
}
%word readu16 1 (fileobj/none -- int/none) read 16-bit unsigned int
{
    uint16_t i;
    if(fread(&i,sizeof(i),1,getf(params,false))>0)
        res->setInt((int)i);
    else
        res->setNone();
}
%word readu32 1 (fileobj/none -- int/none) read 32-bit unsigned int
{
    uint32_t i;
    if(fread(&i,sizeof(i),1,getf(params,false))>0)
        res->setInt((int)i);
    else
        res->setNone();
}


static const char *readstr(FILE *f){
    static char buf[1024];
    char *p=buf;
    for(;;){
        char c = fgetc(f);
        if(c==EOF || c=='\n' || c=='\r' || !c)
            break;
        if(p-buf==1023)
            break;
        *p++=c;
    }
    *p=0;
    return buf;
}

%word readstr 1 (fileobj/none -- str) read string until null/EOL/EOF (max len 1023)
{
    FILE *f = getf(params,false);
    res->setString(readstr(f));
}

%word eof 1 (fileobj/none -- boolean) indicates if EOF has been read
{
    FILE *f = getf(params,false);
    res->setInt(feof(f)?1:0);
}
    
static void doreadlist(FILE *f,PluginValue *res);
static void doreadhash(FILE *f,PluginValue *res);

static bool readval(FILE *f,PluginValue *res){
    
    int type;
    int32_t i;
    float fl;
    if(fread(&type,sizeof(type),1,f)<=0)
        return false;
    switch(type){
    case PV_INT:
        fread(&i,sizeof(i),1,f);
        res->setInt((int)i);
        break;
    case PV_FLOAT:
        fread(&fl,sizeof(fl),1,f);
        res->setFloat(fl);
        break;
    case PV_STRING:
        res->setString(readstr(f));
        break;
    case PV_SYMBOL:
        res->setSymbol(readstr(f));
        break;
    case PV_LIST:
        doreadlist(f,res);
        break;
    case PV_HASH:
        doreadhash(f,res);
        break;
    default:
        throw "file read of unsupported type";
    }
    return true;
}
    
static void doreadlist(FILE *f,PluginValue *res){
    res->setList();
    int n;
    fread(&n,sizeof(n),1,f);
    for(int i=0;i<n;i++){
        PluginValue *v = new PluginValue();
        if(readval(f,v))
            res->addToList(v);
        else {
            delete v;
            throw "premature end of list read";
        }
    }
}
static void doreadhash(FILE *f,PluginValue *res){
    res->setHash();
    int n;
    fread(&n,sizeof(n),1,f);
    for(int i=0;i<n;i++){
        PluginValue *k = new PluginValue();
        if(readval(f,k)){
            PluginValue *v = new PluginValue();
            if(readval(f,v)){
                res->addToList(k);
                res->addToList(v);
            } else {
                delete v;
                throw "badly formed hash in read";
            }
        } else {
            delete k;
            throw "premature end of hash read";
        }
    }
}

%word readlist 1 (fileobj/none -- list) read a binary list (as written by 'write')
{
    res->setList();
    FILE *f = getf(params,false);
    doreadlist(f,res);
}
%word readhash 1 (fileobj/none -- hash) read a binary hash (as written by 'write')
{
    res->setList();
    FILE *f = getf(params,false);
    doreadhash(f,res);
}

%word exists 1 (path -- boolean/none) does a file/directory exist? None indicates some other problem
{
    struct stat b;
    if(stat(params->getString(),&b)==0)
        res->setInt(1);
    else if(errno==ENOENT)
        res->setInt(0);
    else
        res->setNone();
}
    

%word stat 1 (path -- hash/none) read the file statistics, or none if not found
{
    struct stat b;
    if(stat(params->getString(),&b)==0){
        res->setHash();
        res->setHashVal("mode",new PluginValue(b.st_mode));
        res->setHashVal("uid",new PluginValue(b.st_uid));
        res->setHashVal("gid",new PluginValue(b.st_gid));
        res->setHashVal("size",new PluginValue(b.st_size));
        res->setHashVal("atime",new PluginValue(b.st_atime));
        res->setHashVal("mtime",new PluginValue(b.st_mtime));
        res->setHashVal("ctime",new PluginValue(b.st_ctime));
    } else
        res->setNone();
}


%init
{
    printf("Initialising IO plugin, %s %s\n",__DATE__,__TIME__);
}
