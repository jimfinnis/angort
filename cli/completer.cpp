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

// these are global in this file so printCompletions can use them
static int matchlen=0;
static char match[256];


static bool complete(EditLine *el){
    const LineInfo *lf = el_line(el);
    const char* ptr;
    size_t len;
    int res = false;
    
    matchlen=0;
    
    static const char *word_break_chars="\t\n\"\\'@><=;|&{(?! ";
    
    // get the word
    for (ptr = lf->cursor - 1;
         !strchr(word_break_chars,*ptr) &&
         ptr >= lf->buffer; ptr--)
        continue;
    len = lf->cursor - ++ptr;
    
    // make sure it's neither too long nor too short
    if(len>256)return false;
    if(!len)return false;
    
    // now find autocomplete
    
    AutocompleteIterator *c;
    if(el_get(el,EL_CLIENTDATA,&c) || !c)
        return false;
    
    const char *name;
    const char *start = ptr;
    
    // we need to find the shortest valid match, so go through
    // all the options starting with a null string
    // then as soon as we see a match set to that,
    // then decrease the length if we find subsequent shorter
    // matches.
    
    c->first();
    
    while(name = c->next()){
        if(!strncmp(name,ptr,len)){
            if(!matchlen){
                strncpy(match,name,250);
                match[250]=0;
                matchlen=strlen(match);
            } else {
                if(strlen(name)<matchlen)
                    matchlen=strlen(name);
            }
        }
    }
    
    if(matchlen){
        match[matchlen]=0;
        el_deletestr(el,len);
        if(el_insertstr(el,match)==-1)
            res = false;
        else
            res = true;
    }
    
    return res;
}

static void printCompletions(EditLine *el){
    AutocompleteIterator *c;
    if(el_get(el,EL_CLIENTDATA,&c) || !c)
        return; // should never happen
    
    char buf[1024];
    *buf=0;
    
    putchar('\n');
    
    // not very portable. IF it fails, just set linelen
    // to a constant.
    struct winsize ws;
    ioctl(0,TIOCGWINSZ,&ws);
    int linelen = ws.ws_col;
          
    // first pass - calculate maximum ident length,
    // the gap between things.
    
    int gap=0;
    c->first();
    while(const char *name = c->next()){
        if(strlen(name)>gap)
            gap=strlen(name);
    }
    gap++;
    
    
    // second pass - actually print
    c->first();
    int len = 0;
    while(const char *name = c->next()){
        if(!matchlen || !strncmp(name,match,matchlen)){
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
            if(complete(el)){
                putchar('\r'); // OTHERWISE we get the prompt again.
                el_set(el,EL_REFRESH);
            }
            return 1;
        case 2:
            printCompletions(el);
            el_set(el,EL_REFRESH);
            return 1;
        default:
            return 1;
        }
    }
    return rv;
}

void setupAutocomplete(EditLine *el,AutocompleteIterator *i){
    // tell this instance where it should get completion data from
    el_set(el,EL_CLIENTDATA,i);
    el_set(el,EL_GETCFN,tabCheckingCharReader);
}
