/**
 * @file mpc.cpp
 * @brief  Brief description of file.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <mpd/client.h>
#include <mpd/status.h>
#include <mpd/search.h>
#include <mpd/entity.h>
#include <mpd/song.h>

#include <angort/plugins.h>

using namespace angort;

%plugin mpc


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
        mpd_connection_clear_error(mpd);
        throw buf;
    }
};

Connection conn;

static PluginValue *makeSong(const mpd_song *song){
    PluginValue *p = new PluginValue();
    p->setHash();
    p->setHashVal("name",new PluginValue(mpd_song_get_uri(song)));
    p->setHashVal("id",new PluginValue(mpd_song_get_id(song)));
    p->setHashVal("pos",new PluginValue(mpd_song_get_pos(song)));
    p->setHashVal("duration",new PluginValue(mpd_song_get_duration(song)));
    
    for(int i=(int)MPD_TAG_ARTIST;i<(int)MPD_TAG_COUNT;i++){
        const char *s = mpd_song_get_tag(song,(mpd_tag_type)i,0);
        if(s){
            char buf[256];
            const char *tagname = mpd_tag_name((mpd_tag_type)i);
            strcpy(buf,tagname);
            for(char *q=buf;*q;q++)*q=tolower(*q);
            p->setHashVal(buf,new PluginValue(s));
        }
    }
    
    return p;
}

%word connect 2 (hostOrNone portOrZero --) connect to MPD
{
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

%word search 2 (constraintHash exactbool -- songList) search for songs by tags
{
    conn.check();
    
    if(params->type != PV_HASH)
        throw "must be a hash in mpc$search";
    
    bool exact = params[1].getInt()?true:false;
    
    // start the search
    
    if(!mpd_search_db_songs(conn.mpd,exact))
        conn.throwError();
    
    // add constraints
    PluginValue *key,*value;
    for(key = params->v.head;key;key=value->next){
        value = key->next;
        
        if(!mpd_search_add_tag_constraint(conn.mpd,MPD_OPERATOR_DEFAULT,
                                          mpd_tag_name_iparse(key->getString()),
                                          value->getString())){
            mpd_search_cancel(conn.mpd);
            conn.throwError();
        }
    }
    if(!mpd_search_commit(conn.mpd))
        conn.throwError();
        
    
    res->setList();
    mpd_song *song;
    while((song=mpd_recv_song(conn.mpd))!=NULL){
        PluginValue *pv = makeSong(song);
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

%word tags 2 (tag constraints -- list) find all unique values of a tag with given constraints
{
    conn.check();
    
    PluginValue *hash = params+1;
    if(hash->type != PV_HASH)
        throw "must be a hash in mpc$search";
    
    const char *tag = params->getString();
    
    // start the search
    
    mpd_tag_type tagid = mpd_tag_name_iparse(tag);
    if(!mpd_search_db_tags(conn.mpd,tagid))
        conn.throwError();
    
    // add constraints
    PluginValue *key,*value;
    for(key = hash->v.head;key;key=value->next){
        value = key->next;
        
        if(!mpd_search_add_tag_constraint(conn.mpd,MPD_OPERATOR_DEFAULT,
                                          mpd_tag_name_iparse(key->getString()),
                                          value->getString())){
            mpd_search_cancel(conn.mpd);
            conn.throwError();
        }
    }
    if(!mpd_search_commit(conn.mpd))
        conn.throwError();
        
    
    res->setList();
    while(mpd_pair *pair = mpd_recv_pair_tag(conn.mpd,tagid)){
        res->addToList(new PluginValue(pair->value));
        mpd_return_pair(conn.mpd,pair);
    }
    
    if (mpd_connection_get_error(conn.mpd) != MPD_ERROR_SUCCESS)
        conn.throwError();
    
    if (!mpd_response_finish(conn.mpd))
        conn.throwError();
}


%word add 1 (songlist -- ) add songs to queue
{
    conn.check();
    
    mpd_command_list_begin(conn.mpd,false);
    
    if(params->type == PV_OBJ){
        mpd_send_add(conn.mpd,params->getHashVal("name")->getString());
    } else if(params->type == PV_LIST) {
        for(PluginValue *p=params->v.head;p;p=p->next){
            mpd_send_add(conn.mpd,p->getHashVal("name")->getString());
        }
    } else throw("inappropriate type for 'mpc$add'");
    mpd_command_list_end(conn.mpd);
    
    if(!mpd_response_finish(conn.mpd))
        conn.throwError();
}

%word clr 0 (--) clear queue
{
    conn.check();
    if(!mpd_run_clear(conn.mpd))
        conn.throwError();
}

%word list 0 (-- songlist) list the queue
{
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
            PluginValue *pv = makeSong(song);
            res->addToList(pv);
        }
        mpd_entity_free(entity);
    }    
    if(!mpd_response_finish(conn.mpd))
        conn.throwError();
}

%word play 1 (posOrNone --) start playing from a position, or from the current position
{
    conn.check();
    if(params->type == PV_NONE){
        if(!mpd_run_play(conn.mpd))
            conn.throwError();
    } else {
        if(!mpd_run_play_pos(conn.mpd,params->getInt()))
            conn.throwError();
    }
}

%word pause 0 (--) pause the player
{
    conn.check();
    if(!mpd_run_toggle_pause(conn.mpd))
        conn.throwError();
}

%word stop 0 (--) stop the player
{
    conn.check();
    if(!mpd_run_stop(conn.mpd))
        conn.throwError();
}

%word next 0 (--) move to next item in queue
{
    conn.check();
    if(!mpd_run_next(conn.mpd))
        conn.throwError();
}

%word prev 0 (--) move to previous item in queue
{
    conn.check();
    if(!mpd_run_previous(conn.mpd))
        conn.throwError();
}

%word stat 0 (-- hash) produce a status hash
{
    conn.check();
    mpd_status *stat = mpd_run_status(conn.mpd);
    if(!stat)
        conn.throwError();
    res->setHash();
    res->setHashVal("consume",new PluginValue(mpd_status_get_consume(stat)?1:0));
    res->setHashVal("crossfade",new PluginValue((int)mpd_status_get_crossfade(stat)));
    res->setHashVal("elapsed",new PluginValue((int)mpd_status_get_elapsed_time(stat)));
    res->setHashVal("total",new PluginValue((int)mpd_status_get_total_time(stat)));
    res->setHashVal("update",new PluginValue((int)mpd_status_get_update_id(stat)));
    res->setHashVal("volume",new PluginValue((int)mpd_status_get_volume(stat)));
    if(mpd_status_get_error(stat))
        res->setHashVal("error",new PluginValue(mpd_status_get_error(stat)));
    res->setHashVal("queuelength",new PluginValue((int)mpd_status_get_queue_length(stat)));
    res->setHashVal("queueversion",new PluginValue((int)mpd_status_get_queue_version(stat)));
    
    res->setHashVal("id",new PluginValue((int)mpd_status_get_song_id(stat)));
    res->setHashVal("pos",new PluginValue((int)mpd_status_get_song_pos(stat)));
    
    const char *state;
    switch(mpd_status_get_state(stat)){
    case MPD_STATE_UNKNOWN:state="unknown";break;
    case MPD_STATE_STOP:state="stop";break;
    case MPD_STATE_PLAY:state="play";break;
    case MPD_STATE_PAUSE:state="pause";break;
    }
    PluginValue *pv = new PluginValue();
    pv->setSymbol(state);
    res->setHashVal("state",pv);
    
        
    res->setHashVal("random",new PluginValue(mpd_status_get_random(stat)?1:0));
    res->setHashVal("repeat",new PluginValue(mpd_status_get_repeat(stat)?1:0));
    res->setHashVal("single",new PluginValue(mpd_status_get_single(stat)?1:0));
}

%word load 1 (name --) load a playlist
{
    conn.check();
    if(!mpd_run_load(conn.mpd,params[0].getString()))
        conn.throwError();
}

%word save 1 (name --) save a playlist
{
    conn.check();
    if(!mpd_run_save(conn.mpd,params[0].getString()))
        conn.throwError();
}

%word rm 1 (name --) delete a playlist
{
    conn.check();
    if(!mpd_run_rm(conn.mpd,params[0].getString()))
        conn.throwError();
}

%word playlists 0 (-- list) produce a list of playlists
{
    conn.check();
    mpd_send_list_playlists(conn.mpd);
    res->setList();
    while(mpd_playlist *p = mpd_recv_playlist(conn.mpd)){
        res->addToList(new PluginValue(mpd_playlist_get_path(p)));
    }
    mpd_response_finish(conn.mpd);
}

%word setvol 1 (vol --) set the volume
{
    conn.check();
    if(!mpd_run_set_volume(conn.mpd,params[0].getInt()))
        conn.throwError();
}
        


/// this is used for functions which are harder to do!

%word mpc 1 (string --) pass a command string to the standard MPC client
{
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

%init
{
    printf("Initialising MPD client plugin, %s %s\n",__DATE__,__TIME__);
}
