/**
 * @file params.h
 * @brief Parameter file reading utility.
 * This is not part of Angort proper, but a useful utility for adding
 * Angort to C++ programs. Here, the constants for a program (which
 * can also include Angort functions) are stored as a hash inside
 * an Angort file. This class reads and executes this file, which
 * should return the hash, and provides method for reading the hash.
 * 
 * It also imports the angort namespace.
 *
 */

#ifndef __PARAMS_H
#define __PARAMS_H

#include "angort/angort.h"
#include "angort/tokeniser.h"
#include "angort/hash.h"

using namespace angort;

inline void showException(Angort *a,Exception& e){
    printf("Error: %s\n",e.what());
    if(e.ip){
        printf("Error at:");
        a->run->showop(e.ip);
        printf("\n");
    }else
          printf("Last line input: %s\n",a->getLastLine());
    if(e.fatal)
        exit(1);
    
    a->run->clearStack();
}


class Parameters {
    Angort a;
    Value val;
    Hash *hash;
    ArrayList<Value> *list;
    Stack<Hash *,8> hashstack;
    Stack<ArrayList<Value> *,8> liststack;
    
    Value *get(const char *s){
        if(!hash)
            throw RUNT(EX_CORRUPT,"").set("no hash found");
        Value *v = hash->getSym(s);
        if(!v)
            throw RUNT(EX_NOTFOUND,"").set("%s not found in parameters",s);
        return v;
    }
    
    
    /// parse a list of var=val,var=val,var=val
    /// and set globals in Angort from them, as ints, floats or strings
    /// depending on the characters in the value.
    void parseVars(char *s){
#define VT_END 0
#define VT_STRING 1
#define VT_IDENT 2
#define VT_INT 3
#define VT_FLOAT 4
#define VT_DOUBLE 5
#define VT_LONG 6 
#define VT_EQUALS 7
#define VT_COMMA 8
        static TokenRegistry vtoks[]=
        {
            {"*e", VT_END},
            {"*s", VT_STRING},
            {"*i", VT_IDENT},
            {"*n", VT_INT},
            {"*f", VT_FLOAT},
            {"*D", VT_DOUBLE},
            {"*L", VT_LONG},
            {"*c=", VT_EQUALS},
            {"*c,", VT_COMMA},
            {NULL,-10}
        };
            
            
        Tokeniser tok;
        tok.init();
        tok.settokens(vtoks);
        tok.reset(s);
        for(;;){
            int t = tok.getnext();
            if(t==VT_END)
                break;
            else if(t==VT_IDENT){
                char var[128];
                strcpy(var,tok.getstring());
                if(tok.getnext()!=VT_EQUALS)
                    throw RUNT(EX_CORRUPT,"bad varstr: expected '='");
                Value *val = a.findOrCreateGlobalVal(var);
                switch(tok.getnext()){
                case VT_FLOAT:
                    angort::Types::tFloat->set(val,tok.getfloat());
                    break;
                case VT_INT:
                    angort::Types::tInteger->set(val,tok.getint());
                    break;
                case VT_LONG:
                    angort::Types::tLong->set(val,tok.getlong());
                    break;
                case VT_DOUBLE:
                    angort::Types::tDouble->set(val,tok.getdouble());
                    break;
                case VT_STRING:
                case VT_IDENT:
                    angort::Types::tString->set(val,tok.getstring());
                    break;
                default:
                    throw RUNT(EX_CORRUPT,"bad var value");
                }
            }
            else if(t!=VT_COMMA)
                throw RUNT(EX_CORRUPT,"bad varstr: expected varname");
        }
    }
    inline Value *getList(int n){
        if(!list)throw RUNT(EX_TYPE,"not in a pushed list");
        return list->get(n);
    }
    
    
    
    const char *errorstr;
public:
    /// execute the parameters file, which stores the result
    /// on the stack as a hash. Also takes two optional variable strings
    /// (var=val,var=val) for globals to set: one before the param file
    /// runs and one after. These will be damaged (nulls added).
    Parameters(const char *fn,char *prevarstr=NULL,char *postvarstr=NULL){
        hash=NULL; // initialise to invalid
        list=NULL;
        errorstr=NULL;
        if(prevarstr)parseVars(prevarstr);
        // this will cause the entire program to exit
        // if the Angort code calls "quit" or "abort"!
        try {
            a.fileFeed(fn);
        } catch(angort::Exception e){
            showException(&a,e);
            errorstr = strdup(e.what());
            
            return;
        }
        // we copy the obtained hash to avoid premature GC.
        val.copy(a.run->popval());
        if(val.t != Types::tHash){
            throw "parameter file did not leave hash on stack";
        }
        hash = Types::tHash->get(&val);
        if(postvarstr)parseVars(postvarstr);
    }
    ~Parameters(){
        if(errorstr)
            free((void *)errorstr);
    }
    
    /// call after construction to check the file loaded;
    /// if it didn't an error will be returned, otherwise NULL.
    const char *check(){
        return hash ? NULL : errorstr;
    }
    
    /// used for assorted hackage to get a global from Angort
    double getGlobalFloat(const char *s){
        Value *v = a.findOrCreateGlobalVal(s);
        return v->toDouble();
    }
    
    /// used for assorted hackage to set a global in Angort
    void setGlobalFloat(const char *s,double val){
        Value *v = a.findOrCreateGlobalVal(s);
        Types::tDouble->set(v,val);
    }
        
    
    /// does a key exist in the current hash?
    bool exists(const char *s){
        if(hash){
            Value *v = hash->getSym(s);
            return v!=NULL;
        } else 
            return NULL;
    }
    
    /// assuming that "key" is the key to a subhash, go into it
    /// temporarily so we can get values from it
    void push(const char *s){
        Value *v = get(s);
        if(v->t != Types::tHash)
            throw RUNT(EX_TYPE,"").set("%s is not a hash",s);
        hashstack.push(hash);
        hash = Types::tHash->get(v);
    }
    
    void pop(){
        hash = hashstack.pop();
    }
    
    void runFunc(Value *v){
        a.run->runValue(v);
    }
    
    // for processing lists - go into a list with "pushList" and
    // access the items within using getList..(). Remember to popList().
    // You can enter a list inside a list with pushListList(n), and
    // check the size with listCount().
    // You can enter a hash inside a list with pushListHash(n)
    
    void pushList(const char *s){
        Value *v = get(s);
        if(v->t != Types::tList)
            throw RUNT(EX_TYPE,"").set("%s is not a list",s);
        liststack.push(list);
        list = Types::tList->get(v);
    }
    
    void pushListList(int n){
        Value *v = getList(n);
        if(v->t != Types::tList)
            throw RUNT(EX_TYPE,"").set("item %d is not a list",n);
        liststack.push(list);
        list = Types::tList->get(v);
    }
    
    int listCount(){
        if(!list)throw RUNT(EX_TYPE,"not in a pushed list");
        return list->count();
    }
        
    void popList(){
        list = liststack.pop();
    }
    
    
    
    
    /// read a value from the current hash
    double getFloat(const char *s){
        return get(s)->toDouble();
    }
    
    /// read a value from the current hash
    double getInt(const char *s){
        return get(s)->toInt();
    }
    
    double getListFloat(int n){
        return getList(n)->toDouble();
    }
    int getListInt(int n){
        return getList(n)->toInt();
    }
    
    void pushListHash(int n){
        Value *v = getList(n);
        if(v->t != Types::tHash)
            throw RUNT(EX_TYPE,"").set("item %d is not a hash",n);
        hashstack.push(hash);
        hash = Types::tHash->get(v);
    }
                             
    
    /// read a string from the hash into a buffer
    void getString(const char *s,char *out,int max){
        const StringBuffer& r = get(s)->toString();
        strncpy(out,r.get(),max);
    }
    
    /// read a symbol value and look up its string
    const char *getSym(const char *s){
        Value *v = get(s);
        if(v->t != Types::tSymbol)
            throw RUNT(EX_TYPE,"").set("%s is not a symbol",s);
        return Types::tSymbol->get(v);
    }
              
    
    /// read a function which called be called with runFunc
    /// from the current hash
    Value *getFunc(const char *s){
        Value *v = get(s);
        if(!v->t->isCallable())
            throw RUNT(EX_TYPE,"").set("%s is not a function",s);
        return v;
    }
    
    void pushFuncArg(double v){
        a.run->pushDouble(v);
    }
    
    double popFuncResult(){
        return a.run->popDouble();
    }
};


#endif /* __PARAMS_H */
