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

// holds data for this particular completer

class Completer {
    AutocompleteIterator *iter;
    const char *wordBreakChars;
    EditLine *el;
    int matchlen,wordlen;
    char match[256];
    
    
public:
    Completer(EditLine *client, AutocompleteIterator *i,const char *wbc){
        el = client;
        wordBreakChars = wbc;
        iter = i;
        matchlen = 0;
    }
    
    bool complete();
    void printCompletions();
};


bool Completer::complete(){
    const LineInfo *lf = el_line(el);
    const char* ptr;
    size_t len;
    int res = false;
    
    matchlen=0;
    
    // get the word
    for (ptr = lf->cursor - 1;
         !strchr(wordBreakChars,*ptr) &&
         ptr >= lf->buffer; ptr--)
        continue;
    len = lf->cursor - ++ptr;
    
    
    // make sure it's neither too long nor too short
    if(len>256)return false;
    wordlen = len;
    if(!len)return false;
    
    const char *name;
    const char *start = ptr;
    
    // we need to find the shortest valid match, so go through
    // all the options starting with a null string
    // then as soon as we see a match set to that,
    // then decrease the length if we find subsequent shorter
    // matches.
    
    iter->first();
    
    int maxmatchlen=0;
    while(name = iter->next()){
        if(!strncmp(name,ptr,len)){
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
    }
    
    if(matchlen){
        match[matchlen]=0;
        el_deletestr(el,len);
        // if this match is actually the maximum length,
        // it's unambiguous - so we'll add a space. There's
        // room in the buffer.
        if(matchlen==maxmatchlen)strcat(match," ");
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
    
    // don't print completions if we're just at the end
    // of a word and there are no matches. If we don't
    // check this, we'd print the entire completion list
    // (like hitting tab-tab on an empty line)
    if(wordlen && !matchlen){
        putchar('\r');
        return;
    }
    
    putchar('\n');
    
    // not very portable. IF it fails, just set linelen
    // to a constant.
    struct winsize ws;
    ioctl(0,TIOCGWINSZ,&ws);
    int linelen = ws.ws_col;
          
    // first pass - calculate maximum ident length,
    // the gap between things.
    
    int gap=0;
    iter->first();
    while(const char *name = iter->next()){
        if(!strncmp(name,match,matchlen)){
            if(strlen(name)>gap)
                gap=strlen(name);
        }
    }
    gap++;
    
    
    // second pass - actually print
    iter->first();
    int len = 0;
    while(const char *name = iter->next()){
        if(!strncmp(name,match,matchlen)){
            if(len+gap>linelen){
                puts(buf);
                *buf=0;
                len=0;
            }
            memset(buf+len,' ',gap);
            memcpy(buf+len,name,strlen(name));
            buf[len+gap]=0;
            len+=gap;
        }
    }
    puts(buf);
}

// this replaces the default character reader, and
// checks for tabs - we do this so we can check for
// 2 tabs. It then invokes the completion
// match printer or the matcher.

static int tabCheckingCharReader(EditLine *el, char *cp)
{
    Completer *d;
    if(el_get(el,EL_CLIENTDATA,&d))
        d = NULL;
    
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

void setupAutocomplete(EditLine *el,AutocompleteIterator *i,
                       const char *wbc){
    
    // set up a structure; you'll have to call shutdown to delete it
    // tell this instance where it should get completion data from
    
    Completer *d = new Completer(el,i,wbc);
    
    el_set(el,EL_CLIENTDATA,d);
    el_set(el,EL_GETCFN,tabCheckingCharReader);
}

void shutdownAutocomplete(EditLine *el){
    Completer *d;
    if(el_get(el,EL_CLIENTDATA,&d) || !d)
        return;
    el_set(el,EL_CLIENTDATA,NULL);
    delete d;
}
