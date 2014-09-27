#ifndef __ANGORTTOKENISER_H
#define __ANGORTTOKENISER_H

#include <stdio.h>
#include <string.h>


/**
 * @file
 * The Tokeniser and associated classes, which split a line
 * of input into tokens such as keywords, strings, numbers and
 * identifiers.
 */

namespace angort {

class Tokeniser;

struct TokenRegistry
{
    const char *word;
    int token;
};

/// should be a union, really..
struct Token
{
    char s[1024];
    float f;
    int i;
};

/// an interface for a error handler (see seterrorhandler())
class ITokeniserErrorHandler
{
public:
    virtual void HandleTokeniserError(class Tokeniser *t) = 0;
};


/// we can save the tokeniser context into this temporarily
struct TokeniserContext {
    const char *start;
    const char *current;
    const char *end;
    /// the current filename, should be strduped()
    const char *fileName;
    int curtype;
    bool error;
    Token prevval;
    const char *previous;
    int prevtype;
    int prevline;
    bool keywordsOff;
    Token val;
    int line;
    bool trace;
    
};

class Tokeniser
{
public:    
    
    /// this object gets called when the error gets set.
    void seterrorhandler(ITokeniserErrorHandler *h)
    {
        handler = h;
    }
    
    /// initialise
    void init();
    
    /// set the token table
    void settokens(TokenRegistry *k);
    
    
    /// start tokenising a new string.
    /// Takes start and char after end of string (if null, will use strlen())
    void reset(const char *buf,const char *_end=NULL);
    
    /// move onto next token, returning its type
    int getnext();
    
    /// return the current token type
    int getcurrent()
    {
        return curtype;
    }
    
    /// return the error state, which is set when a getnext..() has hit something of the wrong type
    bool iserror() { return error; }
    
    /// get the current token as a raw token
    Token gettoken()
    {
        return val;
    }
    
    /// rewind to the previous token. Can only be done once.
    void rewind();
    
    /// get the value of the next token - must be a float or int
    float getfloat()
    {
        if(curtype==floattoken)
            return val.f;
        else if(curtype==inttoken)
            return (float)val.i;
        else
            return -9999;
    }
    /// get the value of the next token - must be a float or int
    int getint()
    {
        if(curtype==inttoken)
            return val.i;
        else if(curtype==floattoken)
            return (int)val.f;
        else
            return -9999;
    }
    
    /// get the string value of the next token
    char *getstring()
    {
        return val.s;
    }
    
    /// get the char value of the next token
    char getcharacter()
    {
        return val.s[0];
    }
    
    /// get the next item, ensure it's a number, and return it or -9999. If fails, sets the error state.
    float getnextfloat()
    {
        int t = getnext();
        if(t!=floattoken && t!=inttoken){seterror();return -9999;}
        return getfloat();
    }
    /// get the next item, ensure it's a number, and return it or -9999. If fails, sets the error state.
    int getnextint()
    {
        int t = getnext();
        if(t!=floattoken && t!=inttoken){seterror();return -9999;}
        return getint();
    }
    
    /// get the next delimited string, or return false on error
    bool getnextstring(char *out);
    
    /// get the next identifier, or return false on error
    bool getnextident(char *out);
    
    /// get the next identifier, or return false on error; differs
    /// from getnextident() in that keywords will be converted into
    /// identifiers!
    bool getnextidentorkeyword(char *out);
    
    /// return the rest of the line as a string, bypassing the tokeniser
    const char *restofline();
    
    /// skip ahead to a given character. Being a low level routine, this takes a character code - not
    /// a token!
    void skipahead(char c);
    
    /// get the current file name
    const char *getname(){
        return fileName;
    }
    
    /// set the current file name
    void setname(const char *f){
        fileName = f;
    }
    
    /// get current line number
    int getline()
    {
        return line;
    }
    
    /// sometimes we have to do this if the system
    /// uses its own line numbering
    void setline(int l){
        line = l;
    }
    
    /// return the index of the character within the stream
    int getpos() {
        return current-start;
    }
    
    /// turn on debugging
    void settrace(bool t)
    {
        trace = t;
    }
    
    /// if this sequence of characters is reached, skip forwards to the end of the line
    void setcommentlinesequence(const char *s)
    {
        commentlinesequence=s;
        if(s)
            commentlinesequencelen=(int)strlen(s);
        else
            commentlinesequencelen=0;
    }
    
    void saveContext(TokeniserContext *c){
        c->start = start;
        c->current = current;
        c->end = end;
        c->fileName = fileName;
        c->curtype=curtype;
        c->error = error;
        c->prevval=prevval;
        c->previous = previous;
        c->prevtype=prevtype;
        c->prevline=prevline;
        c->keywordsOff=keywordsOff;
        c->val=val;
        c->line=line;
        c->trace=trace;
    }
    
    void restoreContext(TokeniserContext *c){
        start = c->start;
        current = c->current;
        end = c->end;
        fileName = c->fileName;
        curtype=c->curtype;
        error = c->error;
        prevval=c->prevval;
        previous = c->previous;
        prevtype=c->prevtype;
        prevline=c->prevline;
        keywordsOff=c->keywordsOff;
        val=c->val;
        line=c->line;
        trace=c->trace;
    }        
              
    
private:
    void dprintf(const char *s,...);
    
    /// find the token for a keyword if one exists
    int findkeyword(const char *s);
    
    /// skip whitespace
    const char *skipspace(const char *p);
    
    /// set the errorcode and call the handler
    void seterror()
    {
        error = true;
        if(handler)handler->HandleTokeniserError(this);
    }
    
    const char *start;
    const char *current;
    const char *end;
    /// the current filename, should be strduped()
    const char *fileName;
    int curtype;
    bool error;
    Token prevval;
    const char *previous;
    int prevtype;
    int prevline;
    bool keywordsOff;
    Token val;
    int line;
    bool trace;
    
    int chartable[128];
    const char *commentlinesequence;
    int commentlinesequencelen;
    
    TokenRegistry *tokens;
    
    int endtoken,inttoken,stringtoken,identtoken,floattoken;
    ITokeniserErrorHandler *handler;
};

}
#endif /* __TOKENISER_H */
