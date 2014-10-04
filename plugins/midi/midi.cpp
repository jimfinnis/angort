/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <list>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <pthread.h>

#include <angort/angort.h>
#include <angort/hash.h>

using namespace angort;

%name midi
%shared

/// all ports must be linked into this.
std::list<class MidiPort *> portList;

static jack_client_t *jack=NULL;
static void chkjack(){
    if(!jack)
        throw RUNT("midi$open must be called");
}

#define DATABUFSIZE 2048

class MidiPort : public GarbageCollected {
public:
    bool isInput;
    jack_port_t *port;
    MidiPort *next;
    pthread_mutex_t mutex;

    
    uint8_t dataBuffer[DATABUFSIZE];
    int ct;
    
    MidiPort(bool input,const char *name){
        isInput = input;
        port = jack_port_register(jack,name,
                                  JACK_DEFAULT_MIDI_TYPE,
                                  isInput?JackPortIsInput:JackPortIsOutput,0);
        if(!port)
            throw RUNT("failed to register Jack port");
        portList.push_back(this);
        ct=0;
    }
    
    // write a single event to the buffer - an event
    // consists of a byte count followed by a number of bytes.
    
    void write(uint8_t *data,int len){
        if(ct+len+1 >= DATABUFSIZE)
            throw RUNT("out of space in write buffer");
        pthread_mutex_lock(&mutex);
        dataBuffer[ct++]=len;
        memcpy(dataBuffer+ct,data,len);
        ct+=len;
        pthread_mutex_unlock(&mutex);
    }
    
    ~MidiPort(){
        if(port && jack)
            jack_port_unregister(jack,port);
        portList.remove(this);
    }
};

class MidiPortType : public GCType {
public:
    MidiPortType(){
        add("midi","MIDI");
    }
    
    MidiPort *get(Value *v){
        if(v->t != this)
            throw RUNT("not a midiport");
        return (MidiPort *)v->v.gc;
    }
    
    void set(Value *v,MidiPort *p){
        v->clr();
        v->t = this;
        v->v.gc = p;
        incRef(v);
    }
};

static MidiPortType tMidiPort;


// the jack processing callback, called from the jack thread.
static int process(jack_nframes_t nframes, void *arg){
    std::list<MidiPort *>::iterator i;
    
    // go through each port, see which are output and have data
    // waiting to go, and send it.
    
    for(i=portList.begin();i!=portList.end();++i){
        MidiPort *p = *i;
        
        if(!p->isInput){
            void *buf = jack_port_get_buffer(p->port,nframes);
            jack_midi_clear_buffer(buf);
            pthread_mutex_lock(&p->mutex);
            uint8_t *data = p->dataBuffer;
            int idx=0;
            while(idx<p->ct){
                // run through the data, processing the events
                int len = data[idx++];
                jack_midi_event_write(buf,0,data+idx,len);
                idx+=len;
            }
            p->ct=0;
            pthread_mutex_unlock(&p->mutex);
        }
    }
    return 0;
}

static void jack_shutdown(void *arg){
    fprintf(stderr,"Jack terminated the program\n");
    exit(1);
}
    

%word open (name --) create the jack MIDI client with a name
{
    Value *p;
    a->popParams(&p,"s");
    jack = jack_client_open(p->toString().get(),JackNullOption,NULL);
    if(!jack)
        throw RUNT("Jack client creation failed.");
    jack_on_shutdown(jack,jack_shutdown,0);
    jack_set_process_callback(jack,process,0);
    jack_activate(jack);
    
}

%word close (--) close any open jack client
{
    if(jack){
        jack_deactivate(jack);
        jack_client_close(jack);
        jack=NULL;
    }
}    

%word makeout (name -- port) make a MIDI output port
{
    
    Value *p;
    a->popParams(&p,"s");
    chkjack();
    
    MidiPort *port = new MidiPort(false,p->toString().get());
    tMidiPort.set(a->pushval(),port);
}    




%word connect (srcname destname --) connect two Jack ports
{
    Value *p[2];
    a->popParams(p,"ss");
    chkjack();
    switch(int err = jack_connect(jack,p[0]->toString().get(),
                        p[1]->toString().get())){
    case 0:break;
    case EEXIST:
        printf("connection between %s and %s already exists\n",
               p[0]->toString().get(),
               p[1]->toString().get());
    default:
        printf("cannot connect %s to %s : error %d\n",
               p[0]->toString().get(),
               p[1]->toString().get(),err);
               
    }
}

%word on 4 (note vel chan port --)
{
    Value *params[4];
    a->popParams(params,"nnna",&tMidiPort);
    chkjack();
          
    int note = params[0]->toInt();
    int vel = params[1]->toInt();
    int chan = params[2]->toInt()-1; //channels start at 1!
    
    MidiPort *p = tMidiPort.get(params[3]);
    if(p->isInput)
        throw RUNT("attempt to write to input port");
    
    uint8_t data[3];
    data[0]=144+chan;
    data[1]=note;
    data[2]=vel;
    
    p->write(data,3);
}

%word off 3 (note chan port --)
{
    Value *params[4];
    a->popParams(params,"nna",&tMidiPort);
                 
    int note = params[0]->toInt();
    int chan = params[1]->toInt()-1; // channels start at 1!
    
    MidiPort *p = tMidiPort.get(params[2]);
    chkjack();
    if(p->isInput)
        throw RUNT("attempt to write to input port");
    
    uint8_t data[3];
    data[0]=128+chan;
    data[1]=note;
    data[2]=64; // weird, I know.. why does noteoff have velocity?
    p->write(data,3);
}


%init
{
    fprintf(stderr,"Initialising Midi plugin, %s %s\n",__DATE__,__TIME__);
    
}
          
%shutdown
{
    if(jack){
        fprintf(stderr,"closing Jack client.\n");
        jack_client_close(jack);
        jack=NULL;
    }
}
