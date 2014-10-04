/**
 * @file id3.cpp
 * @brief  Brief description of file.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <taglib.h>
#include <fileref.h>
#include <tag.h>

#include <angort/angort.h>
#include <angort/hash.h>

using namespace angort;

%name id3
%shared

inline void setTagStr(Hash *h,const char *k,TagLib::String str){
    h->setSymStr(k,str.toCString(true));
}

%word loadtags (fileName -- hash) load ID3 tags (also stores filename) in hash
{
    Value *p;
    a->popParams(&p,"s");
    
    TagLib::FileRef f(p->toString().get());
    
    p=a->pushval();
    
    if(f.isNull())
        p->setNone();
    else {
        TagLib::Tag *t= f.tag();
        Hash *res = Types::tHash->set(p);
        
        setTagStr(res,"artist",t->artist());
        setTagStr(res,"title",t->title());
        setTagStr(res,"album",t->album());
        setTagStr(res,"comment",t->comment());
        setTagStr(res,"genre",t->genre());
        res->setSymInt("year",t->year());
        res->setSymInt("track",t->track());
        setTagStr(res,"filename",p->toString().get());
    }
}

inline TagLib::String getStr(Hash *hash, const char *name){
    Value *v = hash->getSym(name);
    if(v)
        return TagLib::String(
                              v->toString().get(),
                              TagLib::String::UTF8);
    else
        return TagLib::String::null;
}

inline int getInt(Hash *hash,const char *name){
    Value *v = hash->getSym(name);
    if(v)
        return v->toInt();
    else
        return -1;
}
    

%word savetags (hash --) save ID3 tags as loaded by loadtags (filename is in hash)
{
    Value *p;
    a->popParams(&p,"h");
    Hash *h = Types::tHash->get(p);
    
    Value k;
    Types::tString->set(&k,"filename");
    if(h->find(&k)){
        Value *v = h->getval();
        TagLib::FileRef f(v->toString().get());
        TagLib::Tag *t = f.tag();
    
        t->setArtist(getStr(h,"artist"));
        t->setTitle(getStr(h,"title"));
        t->setAlbum(getStr(h,"album"));
        t->setComment(getStr(h,"comment"));
        t->setGenre(getStr(h,"genre"));
    
        t->setTrack(getInt(h,"track"));
        t->setYear(getInt(h,"year"));
        
        if(!f.save())
            printf("Warning: could not save to file %s",v->toString().get());
    }
    
               
}


%init
{
    fprintf(stderr,"Initialising ID3 plugin, %s %s\n",__DATE__,__TIME__);
}
