/**
 * @file plugins.h
 * @brief  Plugin object definition - include in both
 * Angort core and each plugin.
 */

#ifndef __PLUGINS_H
#define __PLUGINS_H
// plugin value types
#define PV_NORETURN 0 // this means that there is NO RETURN VALUE.
#define PV_INT	    1
#define PV_FLOAT    2
#define PV_STRING   3
#define PV_OBJ      4 // subclass of PluginObject

// for PV_LIST and PV_HASH, the data is a linked list of PluginValue.
// In the hash case, the values come in pairs key->value->key->value.
// The subvalues will be deleted when the main value is deleted.

#define PV_LIST	    5
#define PV_HASH	    6

#define PV_NONE     7 // the return value is NONE.

/// This is the base of a plugin object, which Angort will
/// handle garbage collection for. Note that
/// cycle detection will not be done, so you
/// can't embed other objects safely.

class PluginObject {
public:
    /// The GarbageCollected object which Angort embeds
    /// this in. Set this to NULL on creation to indicate
    /// to Angort that it is a new object.
    class PluginObjectWrapper *wrapper;
    
    PluginObject(){
        wrapper = NULL;
    }
    
    virtual ~PluginObject(){
    }
};


/// this is how a parameter is passed to, and a return value
/// passed from, a plugin. It's a simplified Value.
struct PluginValue {
    int type; //!< a PV_ value
    
    union{
        int i;
        float f;
        const char *s;
        PluginObject *obj;
        PluginValue *head;
    }v;
    
    // used if this PluginValue is contained in a list
    struct PluginValue *next,*tail;
    
    PluginValue(){
        type = PV_NORETURN;
    }
    
    ~PluginValue(){
        PluginValue *p,*q;
        switch(type){
        case PV_STRING:
            free((void *)v.s);
            break;
        case PV_HASH:
        case PV_LIST:
            for(p=v.head;p;p=q){
                q=p->next;
                delete p;
            }
            default:break;
        }
    }
        
    
    int getInt(){
        if(type!=PV_INT)throw "not an int";
        return v.i;
    }
    void setInt(int i){
        v.i = i;
        type = PV_INT;
    }
    float getFloat(){
        if(type!=PV_FLOAT)throw "not a float";
        return v.f;
    }
    void setFloat(float f){
        v.f = f;
        type = PV_FLOAT;
    }
    
    const char *getString(){
        if(type!=PV_STRING)throw "not a string";
        return v.s;
    }
    void setString(const char *s){
        v.s = strdup(s);
        type =PV_STRING;
    }
    void setNone(){
        type = PV_NONE;
    }
    
    void setList(){
        type = PV_LIST;
        v.head = NULL;
    }
    void setHash(){
        type = PV_HASH;
        v.head = NULL;
    }
    
    void addToList(PluginValue *o){
        if(type!=PV_LIST && type!=PV_HASH)throw "not a list";
        o->next=NULL;
        if(!v.head){
            v.head=tail=o;
        } else {
            tail->next = o;
            tail = o;
        }
    }
    
    PluginObject *getObject(){
        if(type!=PV_OBJ)throw "not an object";
        return v.obj;
    }
    void setObject(PluginObject *o){
        v.obj = o;
        type =PV_OBJ;
    }
    
};

/// this is a plugin function; it takes a set
/// of values and returns a value. Be careful
/// with the strings - when passed in, they're on
/// the stack; on return they are in static buffers.
typedef void (*PLUGINFUNC)(PluginValue *res,PluginValue *params);



/// each plugin has a list of these, terminated with
/// a null name.
struct PluginFunc {
public:
    const char *name;
    PLUGINFUNC func;
    int nargs;
};

/// each plugin returns one of these, which has a pointer
/// to the entries above and other data.

struct PluginInfo {
    const char *name; //!< the name of the plugin namespace
    PluginFunc *funcs;
};


/// each plugin exposes a function of this type called "init"
/// which initialises the plugin and returns the above list
typedef PluginInfo *(*PluginInitFunc)();

#endif /* __PLUGINS_H */
