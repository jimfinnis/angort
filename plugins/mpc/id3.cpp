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

#include <angort/plugins.h>

using namespace angort;

%plugin id3

inline void setStr(PluginValue *hash,const char *name,TagLib::String s){
    if(!s.isNull()){
        hash->setHashVal(name,new PluginValue(s.toCString(true)));
    }
}

%word loadtags 1 (fileName -- hash) load ID3 tags (also stores filename) in hash
{
    TagLib::FileRef f(params[0].getString());
    if(f.isNull())
        res->setNone();
    else {
        TagLib::Tag *t= f.tag();
        res->setHash();
        
        setStr(res,"artist",t->artist());
        setStr(res,"title",t->title());
        setStr(res,"album",t->album());
        setStr(res,"comment",t->comment());
        setStr(res,"genre",t->genre());
        res->setHashVal("year",new PluginValue(t->year()));
        res->setHashVal("track",new PluginValue(t->track()));
        res->setHashVal("filename",new PluginValue(params[0].getString()));
    }
}

inline TagLib::String getStr(PluginValue *hash, const char *name){
    PluginValue *v = hash->getHashVal(name);
    if(!v)
        return TagLib::String::null;
    else
        return TagLib::String(v->getString(),TagLib::String::UTF8);
}

%word savetags 1 (hash --) save ID3 tags as loaded by loadtags (filename is in hash)
{
    PluginValue *h = params;
    const char *fn = h->getHashVal("filename")->getString();
    TagLib::FileRef f(fn);
    TagLib::Tag *t = f.tag();
    
    t->setArtist(getStr(h,"artist"));
    t->setTitle(getStr(h,"title"));
    t->setAlbum(getStr(h,"album"));
    t->setComment(getStr(h,"comment"));
    t->setGenre(getStr(h,"genre"));
    
    t->setTrack(h->getHashVal("track")->getInt());
    t->setYear(h->getHashVal("year")->getInt());
    
    if(!f.save())
        printf("Warning: could not save to file %s",fn);
               
}


%init
{
    printf("Initialising ID3 plugin, %s %s\n",__DATE__,__TIME__);
}
