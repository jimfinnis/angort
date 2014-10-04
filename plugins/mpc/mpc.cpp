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

#include <angort/angort.h>
#include <angort/hash.h>

using namespace angort;

%name mpc
%shared

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
        disconnect();
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
            throw RUNT("MPD not connected");
    }
    
    void throwError(){
        char buf[1024];
        sprintf(buf,"MPD error: %s",mpd_connection_get_error_message(mpd));
        mpd_connection_clear_error(mpd);
        throw RUNT("").set(buf);
    }
};

Connection conn;


void makeSong(Value *out, const mpd_song *song){
    Hash *h = Types::tHash->set(out);
    
    
    h->setSymStr("name",mpd_song_get_uri(song));
    h->setSymInt("id",mpd_song_get_id(song));
    h->setSymInt("pos",mpd_song_get_pos(song));
    h->setSymInt("duration",mpd_song_get_duration(song));
    
    for(int i=(int)MPD_TAG_ARTIST;i<(int)MPD_TAG_COUNT;i++){
        const char *s = mpd_song_get_tag(song,(mpd_tag_type)i,0);
        if(s){
            char buf[256];
            const char *tagname = mpd_tag_name((mpd_tag_type)i);
            strcpy(buf,tagname);
            for(char *q=buf;*q;q++)*q=tolower(*q);
            
            h->setSymStr(buf,s);
        }
    }
}

%word connect (hostOrNone portOrZero --) connect to MPD
{
    Value *params[2];
    
    a->popParams(params,"An",Types::tString);
    
    if(conn.mpd)
        conn.disconnect();
    
    const char *error = conn.connect(params[0]->isNone()?NULL:
                                       params[0]->toString().get(),
                                     params[1]->toInt());
    if(error)
        throw RUNT("").set(error);
}

%word search (constraintHash exactbool -- songList) search for songs by tags
{
    Value *params[2];
    
    a->popParams(params,"hn");
    conn.check();
    
    bool exact = params[1]->toInt()?true:false;
    
    // start the search
    
    if(!mpd_search_db_songs(conn.mpd,exact))
        conn.throwError();
    
    // add constraints
    Hash *h = Types::tHash->get(params[0]);
    HashKeyIterator iter(h);
    
    for(iter.first();!iter.isDone();iter.next()){
        Value *key = iter.current();
        Value *val = iter.curval();
        if(!mpd_search_add_tag_constraint(conn.mpd,MPD_OPERATOR_DEFAULT,
                                          mpd_tag_name_iparse(key->toString().get()),
                                          val->toString().get())){
            mpd_search_cancel(conn.mpd);
            conn.throwError();
        }
    }
    if(!mpd_search_commit(conn.mpd))
        conn.throwError();
    
    Value *result = a->pushval();
    ArrayList<Value> *list = Types::tList->set(result);
    
    mpd_song *song;
    while((song=mpd_recv_song(conn.mpd))!=NULL){
        makeSong(list->append(),song);
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
    result->clr(); // should delete the list.
    throw mpd_connection_get_error_message(conn.mpd);
}

%word tags (tag constraints -- list) find all unique values of a tag with given constraints
{
    Value *params[2];
    a->popParams(params,"sh");
    
    conn.check();
    
    const StringBuffer& tag = params[0]->toString();
    Hash *h = Types::tHash->get(params[1]);
    
    // start the search
    
    mpd_tag_type tagid = mpd_tag_name_iparse(tag.get());
    if(!mpd_search_db_tags(conn.mpd,tagid))
        conn.throwError();
    
    // add constraints
    HashKeyIterator iter(h);
    for(iter.first();!iter.isDone();iter.next()){
        Value *key = iter.current();
        Value *val = iter.curval();
        
        if(!mpd_search_add_tag_constraint(conn.mpd,MPD_OPERATOR_DEFAULT,
                                          mpd_tag_name_iparse(key->toString().get()),
                                          val->toString().get())){
            mpd_search_cancel(conn.mpd);
            conn.throwError();
        }
    }
    if(!mpd_search_commit(conn.mpd))
        conn.throwError();
    
    Value *result = a->pushval();
    ArrayList<Value> *list = Types::tList->set(result);
    
    while(mpd_pair *pair = mpd_recv_pair_tag(conn.mpd,tagid)){
        Value *v = list->append();
        Types::tString->set(v,pair->value);
        mpd_return_pair(conn.mpd,pair);
    }
    
    if (mpd_connection_get_error(conn.mpd) != MPD_ERROR_SUCCESS){
        conn.throwError();
        result->clr();
    }
    
    if (!mpd_response_finish(conn.mpd)){
        conn.throwError();
        result->clr();
    }
}

// given a value which is a hash, get the name field and send this
// as an "add" command.
void sendAddOfNameInHash(Value *v){
    Value k;
    
    if(v->t != Types::tHash)
        throw RUNT("song must be a hash");
    
    Hash *h = Types::tHash->get(v);
    
    if(Value *name = h->getSym("name"))
        mpd_send_add(conn.mpd, name->toString().get());
    else
        throw RUNT("song hash must contain name field");
}

%word add (songlist -- ) add songs to queue
{
    Value *v = a->popval();
    
    conn.check();
    
    mpd_command_list_begin(conn.mpd,false);
    
    if(v->t == Types::tHash){
        sendAddOfNameInHash(v);
    } else if(v->t == Types::tList) {
        ArrayListIterator<Value> iter(Types::tList->get(v));
        for(iter.first();!iter.isDone();iter.next()){
            sendAddOfNameInHash(iter.current());
        }
    } else throw("inappropriate type for 'mpc$add'");
    mpd_command_list_end(conn.mpd);
    
    if(!mpd_response_finish(conn.mpd))
        conn.throwError();
}

%word clr (--) clear queue
{
    conn.check();
    if(!mpd_run_clear(conn.mpd))
        conn.throwError();
}

%word list (-- songlist) list the queue
{
    conn.check();
    mpd_command_list_begin(conn.mpd,true);
    mpd_send_list_queue_meta(conn.mpd);
    mpd_command_list_end(conn.mpd);
    
    Value *res = a->pushval();
    ArrayList<Value> *list = Types::tList->set(res);
    
    for(mpd_entity* entity = mpd_recv_entity(conn.mpd);
        entity;
        entity = mpd_recv_entity(conn.mpd)) {
        if(mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
            makeSong(list->append(),mpd_entity_get_song(entity));
        }
        mpd_entity_free(entity);
    }    
    if(!mpd_response_finish(conn.mpd))
        conn.throwError();
}

%word play (posOrNone --) start playing from a position, or from the current position
{
    Value *p;
    a->popParams(&p,"A",Types::tInteger);
    
    conn.check();
    if(p->isNone()) {
        if(!mpd_run_play(conn.mpd))
            conn.throwError();
    } else {
        if(!mpd_run_play_pos(conn.mpd,p->toInt()))
            conn.throwError();
    }
}

%word pause (--) pause the player
{
    conn.check();
    if(!mpd_run_toggle_pause(conn.mpd))
        conn.throwError();
}

%word stop (--) stop the player
{
    conn.check();
    if(!mpd_run_stop(conn.mpd))
        conn.throwError();
}

%word next (--) move to next item in queue
{
    conn.check();
    if(!mpd_run_next(conn.mpd))
        conn.throwError();
}

%word prev (--) move to previous item in queue
{
    conn.check();
    if(!mpd_run_previous(conn.mpd))
        conn.throwError();
}

%word stat (-- hash) produce a status hash
{
    conn.check();
    mpd_status *stat = mpd_run_status(conn.mpd);
    if(!stat)
        conn.throwError();
    
    Value *res = a->pushval();
    Hash *h = Types::tHash->set(res);
    
    h->setSymInt("consume",mpd_status_get_consume(stat));
    h->setSymInt("crossfade",mpd_status_get_crossfade(stat));
    h->setSymInt("elapsed",mpd_status_get_elapsed_time(stat));
    h->setSymInt("total",mpd_status_get_total_time(stat));
    h->setSymInt("update",mpd_status_get_update_id(stat));
    h->setSymInt("volume",mpd_status_get_volume(stat));
    if(mpd_status_get_error(stat))
        h->setSymStr("error",mpd_status_get_error(stat));
    h->setSymInt("queuelength",mpd_status_get_queue_length(stat));
    h->setSymInt("queueversion",mpd_status_get_queue_version(stat));
    
    h->setSymInt("id",mpd_status_get_song_id(stat));
    h->setSymInt("pos",mpd_status_get_song_pos(stat));
    
    // the state is a symbol, which is slightly fiddly.
    const char *state;
    switch(mpd_status_get_state(stat)){
    case MPD_STATE_UNKNOWN:state="unknown";break;
    case MPD_STATE_STOP:state="stop";break;
    case MPD_STATE_PLAY:state="play";break;
    case MPD_STATE_PAUSE:state="pause";break;
    }
    h->setSymSym("state",state);
    
    h->setSymInt("random",mpd_status_get_random(stat)?1:0);
    h->setSymInt("repeat",mpd_status_get_repeat(stat)?1:0);
    h->setSymInt("single",mpd_status_get_single(stat)?1:0);
}

%word load (name --) load a playlist
{
    Value *p;
    a->popParams(&p,"s");
    
    conn.check();
    if(!mpd_run_load(conn.mpd,p->toString().get()))
        conn.throwError();
}

%word save (name --) save a playlist
{
    Value *p;
    a->popParams(&p,"s");
    
    conn.check();
    if(!mpd_run_save(conn.mpd,p->toString().get()))
        conn.throwError();
}

%word rm (name --) delete a playlist
{
    Value *p;
    a->popParams(&p,"s");
    
    conn.check();
    if(!mpd_run_rm(conn.mpd,p->toString().get()))
        conn.throwError();
}

%word playlists (-- list) produce a list of playlists
{
    conn.check();
    mpd_send_list_playlists(conn.mpd);
    
    ArrayList<Value> *list = Types::tList->set(a->pushval());
    
    while(mpd_playlist *p = mpd_recv_playlist(conn.mpd)){
        Types::tString->set(list->append(),mpd_playlist_get_path(p));
    }
    mpd_response_finish(conn.mpd);
}

%word setvol (vol --) set the volume
{
    Value *p;
    a->popParams(&p,"n");
    
    conn.check();
    if(!mpd_run_set_volume(conn.mpd,p->toInt()))
        conn.throwError();
}
        


/// this is used for functions which are harder to do!

%word mpc (string -- outlist/none) pass a command string to the standard MPC client
{
    FILE *fp;
    
    Value *p;
    a->popParams(&p,"s");
    
    char buf[1024];
    snprintf(buf,1024,"/usr/bin/mpc %s",p->toString().get());
    
    fp = popen(buf,"r");
    
    Value *res = a->pushval();
    if(fp!=NULL){
        ArrayList<Value> *list = Types::tList->set(res);
        while(fgets(buf,sizeof(buf)-1,fp)!=NULL){
            buf[strlen(buf)-1]=0; // strip trailing LF
            Types::tString->set(list->append(),buf);
        }
        pclose(fp);   
    } else 
        res->setNone();
}

#define MIN3(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))

int levenshtein(const char *s1, const char *s2) {
    unsigned int x, y, s1len, s2len;
    s1len = strlen(s1);
    s2len = strlen(s2);
    unsigned int matrix[s2len+1][s1len+1];
    matrix[0][0] = 0;
    for (x = 1; x <= s2len; x++)
        matrix[x][0] = matrix[x-1][0] + 1;
    for (y = 1; y <= s1len; y++)
        matrix[0][y] = matrix[0][y-1] + 1;
    for (x = 1; x <= s2len; x++)
        for (y = 1; y <= s1len; y++)
            matrix[x][y] = MIN3(matrix[x-1][y] + 1, matrix[x][y-1] + 1,
                                matrix[x-1][y-1] +
                                (tolower(s1[y-1]) == tolower(s2[x-1]) ? 0 : 1));
 
    return(matrix[s2len][s1len]);
}

%word strneareq (a b--bool) compares two strings for near equality
{
    Value *params[2];
    a->popParams(params,"vv");
    
    const StringBuffer &p = params[0]->toString();
    const StringBuffer &q = params[1]->toString();
    
    int x = levenshtein(p.get(),q.get());
    
    int lenp = strlen(p.get());
    int lenq = strlen(q.get());
    int mn = (lenp<lenq)?lenp:lenq;
    
    float rat = (float)x / (float)mn;
    
    Types::tInteger->set(a->pushval(),(rat<0.0001)?1:0);
} 

%init
{
    fprintf(stderr,"Initialising MPD client plugin, %s %s\n",__DATE__,__TIME__);
}

%shutdown
{
    conn.disconnect();
}
