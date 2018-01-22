/**
 * @file completer.cpp
 * @brief  Brief description of file.
 *
 */

#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <histedit.h>
#include "completer.h"

#include <map>

namespace completer {

// holds data for this particular completer

class Completer {
    Iterator *defaultiter;
    const char *wordBreakChars;
    EditLine *el;
    char match[256];
    
    std::map<int,Iterator *> argiters;
    
    // if there's an autocomplete iterator for argc, return it
    // otherwise return the default
    Iterator *getIterator(int argc){
        std::map<int,Iterator *>::iterator it
              = argiters.find(argc);
        
        return (it==argiters.end()) ? defaultiter : it->second;
    }
    
public:
    Completer(EditLine *client, Iterator *i,const char *wbc){
        el = client;
        wordBreakChars = wbc;
        defaultiter = i;
    }
    
    void setArgIterator(int i,Iterator *iter){
        argiters[i] = iter;
    }
    
    bool complete();
    void printCompletions();

// get the completer for this EditLine or null (which should
// never happen)

    inline static Completer *get(EditLine *el){
        Completer *d;
        if(el_get(el,EL_CLIENTDATA,&d))
            d = NULL;
        return d;
    }
};


bool Completer::complete(){
    const LineInfo *lf = el_line(el);
    int res = false;
    int matchlen=0;
    
    // count the argument number
    int argc=0;
    for(const char *p=lf->buffer;*p && p!=lf->cursor;p++){
        if(p==lf->cursor || strchr(wordBreakChars,*p)){
            argc++;
        }
    }
    
    // get the word
    const char *ptr;
    for(ptr=lf->cursor-1;!strchr(wordBreakChars,*ptr)&&
        ptr>=lf->buffer;ptr--)continue;
    size_t len = lf->cursor - ++ptr;
    
    // make sure it's neither too long nor too short
    if(len>256)return false;
    if(!len)return false;
    
    const char *name;
    const char *start = ptr;
    
    // work out which iterator we should use
    Iterator *iter = getIterator(argc);
    
    // try to replace the entire word (for tilde replacement in files,
    // etc)
    const char *modres = iter->modString(ptr,len);
    if(modres){
        // delete the entire word, insert the replacement
        el_deletestr(el,len);
        el_insertstr(el,modres);
        len = strlen(modres); // get the new word length
        free((void *)modres); // free the result
        putchar('\r'); // OTHERWISE we get the prompt again.
        el_set(el,EL_REFRESH);
    }
    
    // we need to find the shortest valid match, so go through
    // all the options starting with a null string
    // then as soon as we see a match set to that,
    // then decrease the length if we find subsequent shorter
    // matches.
    
    iter->first(ptr,len);
    
    int maxmatchlen=0;
    while(name = iter->next()){
        if(!matchlen){
            strncpy(match,name,250);
            match[250]=0;
            matchlen=maxmatchlen=strlen(match);
        } else {
            // look for character count in common with existing
            // match
            int i=0;
            const char *a=match;
            const char *b=name;
            while(*a && *b && *a==*b){i++;a++;b++;}
            if(i<matchlen)matchlen=i;
            if(strlen(name)>maxmatchlen)maxmatchlen=strlen(name);
        }
    }
    
    if(matchlen){
        match[matchlen]=0;
        el_deletestr(el,len);
        // if this match is actually the maximum length,
        // it's unambiguous - so we'll add a space. There's
        // room in the buffer. We only do this if the selected
        // iterator returns doSpacePadding() true (the default)
        if(matchlen==maxmatchlen && iter->doSpacePadding())
            strcat(match," ");
        if(el_insertstr(el,match)==-1)
            res = false;
        else
            res = true;
    }
    
    return res;
}

void Completer::printCompletions() {
    char buf[1024];
    *buf=0;
    const LineInfo *lf = el_line(el);
    
    // count the argument number
    int argc=0;
    for(const char *p=lf->buffer;*p && p!=lf->cursor;p++){
        if(p==lf->cursor || strchr(wordBreakChars,*p)){
            argc++;
        }
    }
    
    // get the word
    const char *ptr;
    for(ptr=lf->cursor-1;!strchr(wordBreakChars,*ptr)&&
        ptr>=lf->buffer;ptr--)continue;
    size_t len = lf->cursor - ++ptr;
    
    // work out which iterator we should use
    Iterator *iter = getIterator(argc);
    
    putchar('\n');
    
    // not very portable. IF it fails, just set linelen
    // to a constant.
    struct winsize ws;
    ioctl(0,TIOCGWINSZ,&ws);
    int linelen = ws.ws_col;
    
    // first pass - calculate maximum ident length,
    // the gap between things.
    
    int gap=0;
    iter->first(ptr,len);
    while(const char *name = iter->next()){
        if(strlen(name)>gap)
            gap=strlen(name);
    }
    gap++;
    
    
    // second pass - actually print
    iter->first(ptr,len);
    int clen = 0;
    while(const char *name = iter->next()){
        if(clen+gap>linelen){
            puts(buf);
            *buf=0;
            clen=0;
        }
        memset(buf+clen,' ',gap);
        memcpy(buf+clen,name,strlen(name));
        buf[clen+gap]=0;
        clen+=gap;
    }
    puts(buf);
}

// this replaces the default character reader, and
// checks for tabs - we do this so we can check for
// 2 tabs. It then invokes the completion
// match printer or the matcher.

static int tabCheckingCharReader(EditLine *el, wchar_t *cp)
{
    Completer *d = Completer::get(el);
    
    int rv;
    static int consecutiveTabCount=0;
    FILE *fd = NULL;
    el_get(el, EL_GETFP, 0, &fd);
    rv = fgetc(fd);
    if (rv > 0){
        *cp = rv;
        if(rv == 9)
            consecutiveTabCount++;
        else
            consecutiveTabCount=0;
        switch(consecutiveTabCount){
        case 1:
            if(d && d->complete()){
                putchar('\r'); // OTHERWISE we get the prompt again.
                el_set(el,EL_REFRESH);
            }
            return 1;
        case 2:
            if(d){
                d->printCompletions();
                el_set(el,EL_REFRESH);
            }
            return 1;
        default:
            return 1;
        }
    }
    return rv;
}

void setup(EditLine *el,Iterator *i,
                       const char *wbc){
    
    // set up a structure; you'll have to call shutdown to delete it
    // tell this instance where it should get completion data from
    
    Completer *d = new Completer(el,i,wbc);
    
    el_set(el,EL_CLIENTDATA,d);
    el_set(el,EL_GETCFN,tabCheckingCharReader);
}

void shutdown(EditLine *el){
    Completer *d = Completer::get(el);
    el_set(el,EL_CLIENTDATA,NULL);
    el_set(el,EL_GETCFN,EL_BUILTIN_GETCFN);
    if(d)delete d;
}

void setArgIterator(EditLine *el,int arg,Iterator *i){
    Completer *d = Completer::get(el);
    d->setArgIterator(arg,i);
}


} // namespace
