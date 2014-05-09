/**
 * @file plugins.h
 * @brief  Plugin object definition - include in both
 * Angort core and each plugin.
 */

#ifndef __PLUGINS_H
#define __PLUGINS_H
// plugin value types
#define PV_NONE     0
#define PV_INT	    1
#define PV_FLOAT    2
#define PV_STRING   3


/// this is how a parameter is passed to, and a return value
/// passed from, a plugin. It's a simplified Value.
union PluginValue {
    int type; //!< a PV_ value
    int i;
    float f;
    const char *s;
};

/// this is a plugin function; it takes a set
/// of values and returns another set. Be careful
/// with the strings - when passed in, they're on
/// the stack; on return they are in static buffers.
typedef PluginValue (*PLUGINFUNC)(PluginValue *params);



/// each plugin has a list of these, terminated with
/// one containing a negative number of args and a
/// null pointer.
struct PluginEntry {
public:
    int nargs;
    PLUGINFUNC func;
};

/// each plugin exposes a function of this type called "init"
/// which initialises the plugin and returns the above list
typedef PluginEntry *(*PluginInitFunc)();

#endif /* __PLUGINS_H */
