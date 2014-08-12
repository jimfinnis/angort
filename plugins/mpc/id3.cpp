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

inline void setStr(Hash *h, const char *key,TagLib::String s){
    Value k,v;
    if(!s.isNull()){
        Types::tSymbol->set(&k,SymbolType::getSymbol(key));
        Types::tString->set(&v,s.toCString(true));
        h->set(&k,&v);
    }
}

inline void setInt(Hash *h, const char *key,uint32_t value){
    Value k,v;
    Types::tSymbol->set(&k,SymbolType::getSymbol(key));
    Types::tInteger->set(&v,(int)value);
    h->set(&k,&v);
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
        
        setStr(res,"artist",t->artist());
        setStr(res,"title",t->title());
        setStr(res,"album",t->album());
        setStr(res,"comment",t->comment());
        setStr(res,"genre",t->genre());
        setInt(res,"year",t->year());
        setInt(res,"track",t->track());
        setStr(res,"filename",p->toString().get());
    }
}

inline TagLib::String getStr(Hash *hash, const char *name){
    Value k;
    Types::tString->set(&k,name);
    
    if(hash->find(&k))
        return TagLib::String(
                              hash->getval()->toString().get(),
                              TagLib::String::UTF8);
    else
        return TagLib::String::null;
}

inline int getInt(Hash *hash,const char *name){
    Value k;
    Types::tString->set(&k,name);
    
    if(hash->find(&k))
        return hash->getval()->toInt();
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
    printf("Initialising ID3 plugin, %s %s\n",__DATE__,__TIME__);
}
