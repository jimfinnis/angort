/**
 * @file mpc.cpp
 * @brief  Brief description of file.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpd/client.h>
#include <mpd/status.h>
#include <mpd/search.h>
#include <mpd/entity.h>
#include <mpd/song.h>

#include "../include/plugins.h"

const char *todo = "Tidy up song data item access. Some form of hash?";

class SongObject : public PluginObject {
public:
    const char *name;
    int duration;
    int id; // ID in playlist or 0 if not
    SongObject(const mpd_song *song){
        name = strdup(mpd_song_get_uri(song));
        duration = mpd_song_get_duration(song);
        id = mpd_song_get_id(song);
    }
    
    /// needs to be virtual because we destruct through
    /// the PluginObject.
    virtual ~SongObject(){
        free((void *)name);
    }
};

/// song access words
static void nameFunc(PluginValue *res,PluginValue *params){
    if(params[0].type != PV_OBJ)
        res->setNone();
    else
        res->setString(((SongObject*)params[0].getObject())->name);
}
static void durationFunc(PluginValue *res,PluginValue *params){
    if(params[0].type != PV_OBJ)
        res->setNone();
    else
        res->setInt(((SongObject*)params[0].getObject())->duration);
}

static void idFunc(PluginValue *res,PluginValue *params){
    if(params[0].type != PV_OBJ)
        res->setNone();
    else
        res->setInt(((SongObject*)params[0].getObject())->id);
}



/// handles the connection. This should be created by "connect",
/// and disconnection will be handled by the destructor, which
/// runs on exit.

class Connection {
    char *error;
    void seterror(const char *s){
        if(error)free(error);
        error=strdup(s);
    }
    
public:
    mpd_connection *mpd;
    Connection(){
        error = NULL;
        mpd=NULL;
    }
    ~Connection(){
        if(error)free(error);
        if(mpd)
            mpd_connection_free(mpd);
    }
    
    const char *geterror(){
        return error;
    }
    
    /// returns NULL or an error. host can be null for default,
    /// port zero for default.
    const char *connect(const char *host, int port=0){
        mpd = mpd_connection_new(host,port,2000); // 2 second timeout
        if(mpd_connection_get_error(mpd)!=MPD_ERROR_SUCCESS){
            seterror(mpd_connection_get_error_message(mpd));
            mpd_connection_free(mpd);
            mpd=NULL;
            return error;
        }
        return NULL;
    }
    void disconnect(){
        if(error)free(error);
        if(mpd)
            mpd_connection_free(mpd);
        error=NULL;mpd=NULL;
    }
    
    void check(){
        if(!mpd)
            throw("MPD not connected");
    }
    
    void throwError(){
        char buf[1024];
        sprintf(buf,"MPD error: %s",mpd_connection_get_error_message(mpd));
        throw buf;
    }
};

Connection conn;


static void connectFunc(PluginValue *res,PluginValue *params){
    if(conn.mpd)
        conn.disconnect();
    
    
    const char *host;
    if(params[0].type == PV_NONE)
        host = NULL;
    else
        host = params[0].getString();
    int port = params[1].getInt();
    
    const char *error = conn.connect(host,port);
    if(error)
        throw error;
}

static void searchFunc(PluginValue *res,PluginValue *params){
    conn.check();
    
    if(params->type != PV_HASH)
        throw "must be a hash in mpc$search";
    
    // start the search
    if(!mpd_search_db_songs(conn.mpd,false))
        throw mpd_connection_get_error_message(conn.mpd);
    
    // add constraints
    PluginValue *key,*value;
    for(key = params->v.head;key;key=value->next){
        value = key->next;
        
        if(!mpd_search_add_tag_constraint(conn.mpd,MPD_OPERATOR_DEFAULT,
                                          mpd_tag_name_iparse(key->getString()),
                                          value->getString()))
            conn.throwError();
    }
    if(!mpd_search_commit(conn.mpd))
        throw mpd_connection_get_error_message(conn.mpd);
    
    res->setList();
    mpd_song *song;
    while((song=mpd_recv_song(conn.mpd))!=NULL){
        PluginValue *pv = new PluginValue();
        SongObject *o = new SongObject(song);
        
        pv->setObject(o);
        res->addToList(pv);
        mpd_song_free(song);
    }
    if (mpd_connection_get_error(conn.mpd) != MPD_ERROR_SUCCESS) {
        goto error;
    }
    
    if (!mpd_response_finish(conn.mpd)) {
        goto error;
    }    
    
    return;
    // ugly, yes.
error:
    if(res->type == PV_LIST){
        PluginValue *q;
        for(PluginValue *p=res->v.head;p;p=q){
            q=p->next;
            delete p;
        }
    }
    throw mpd_connection_get_error_message(conn.mpd);
}

static void addFunc(PluginValue *res,PluginValue *params){
    conn.check();
    
    mpd_command_list_begin(conn.mpd,false);
    
    if(params->type == PV_OBJ){
        mpd_send_add(conn.mpd,((SongObject *)params->getObject())->name);
    } else if(params->type == PV_LIST) {
        for(PluginValue *p=params->v.head;p;p=p->next){
            mpd_send_add(conn.mpd,((SongObject *)p->getObject())->name);
        }
    } else throw("inappropriate type for 'mpc$add'");
    mpd_command_list_end(conn.mpd);
    
    if(!mpd_response_finish(conn.mpd))
        conn.throwError();
}

static void clearFunc(PluginValue *res,PluginValue *params){
    conn.check();
    if(!mpd_run_clear(conn.mpd))
        conn.throwError();
}

static void listFunc(PluginValue *res,PluginValue *params){
    conn.check();
    mpd_command_list_begin(conn.mpd,true);
    mpd_send_list_queue_meta(conn.mpd);
    mpd_command_list_end(conn.mpd);
    
    res->setList();
    for(mpd_entity* entity = mpd_recv_entity(conn.mpd);
        entity;
        entity = mpd_recv_entity(conn.mpd)) {
        if(mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
            const mpd_song* song = mpd_entity_get_song(entity);
            PluginValue *pv = new PluginValue();
            SongObject *o = new SongObject(song);
            pv->setObject(o);
            res->addToList(pv);
        }
        mpd_entity_free(entity);
    }    
    if(!mpd_response_finish(conn.mpd))
        conn.throwError();
}

static void playFunc(PluginValue *res,PluginValue *params){
    conn.check();
    if(params->type == PV_NONE){
        if(!mpd_run_play(conn.mpd))
            conn.throwError();
    } else {
        if(!mpd_run_play_pos(conn.mpd,params->getInt()))
            conn.throwError();
    }
}
static void pauseFunc(PluginValue *res,PluginValue *params){
    conn.check();
    if(!mpd_run_toggle_pause(conn.mpd))
        conn.throwError();
}
static void stopFunc(PluginValue *res,PluginValue *params){
    conn.check();
    if(!mpd_run_stop(conn.mpd))
        conn.throwError();
}
static void nextFunc(PluginValue *res,PluginValue *params){
    conn.check();
    if(!mpd_run_next(conn.mpd))
        conn.throwError();
}
static void prevFunc(PluginValue *res,PluginValue *params){
    conn.check();
    if(!mpd_run_previous(conn.mpd))
        conn.throwError();
}



/// this is used for functions which are harder to do!

static void mpcFunc(PluginValue *res,PluginValue *params){
    FILE *fp;
    
    char buf[1024];
    snprintf(buf,1024,"/usr/bin/mpc %s",params[0].getString());
    fp = popen(buf,"r");
    res->setList();
    if(fp!=NULL){
        while(fgets(buf,sizeof(buf)-1,fp)!=NULL){
            PluginValue *pv = new PluginValue();
            buf[strlen(buf)-1]=0; // strip trailing LF
            pv->setString(buf);
            res->addToList(pv);
        }
        pclose(fp);   
    } else 
        res->setNone();
}

static PluginFunc funcs[]= {
    // song data access
    {"name",nameFunc,1}, // (song -- uri)
    {"duration",durationFunc,1}, // (song -- duration)
    {"id",idFunc,1}, // (song -- duration)
    
    {"connect",connectFunc,2}, // (hostOrNone portOrZero --)
    
    // searches, returning song objects
    {"search",searchFunc,1}, // (constrainthash -- list)
    
    // playlist manipulation
    {"add",addFunc,1}, // (songlist--)
    {"clear",clearFunc,0},
    {"list",listFunc,0},
    
    {"play",playFunc,1}, // (noneOrID --)
    {"pause",pauseFunc,0},
    {"stop",stopFunc,0},
    {"next",nextFunc,0},
    {"prev",prevFunc,0},
    
    // shell command function
    {"mpc",mpcFunc,1},
    {NULL,NULL,-1}
};


static PluginInfo info = {
    "mpc",funcs
};

extern "C" PluginInfo *init(){
    printf("Initialising MPC plugin\n%s\n",todo);
    return &info;
}

