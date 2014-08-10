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
#include <string>
#include <vector>
#include <RtMidi.h>
#include <angort/plugins.h>

using namespace angort;

static AngortPluginInterface *api;

class Out : public PluginObject {
public:
    RtMidiOut *out;
    
    Out(){
        out = new RtMidiOut();
    }
    
    void close(){
        printf("Closing port\n");
        if(out)delete out;
        out=NULL;
    }
        
    
    ~Out(){
        if(out)
            delete out;
    }
};


%plugin midi

%word makeout 0 (-- port) create an unconnected output
{
    Out *o = new Out();
    res->setObject(o);
}
    

%word openout 2 (n port --) link an output to a port
{ 
    int n = params->getInt();
    Out *o = (Out *)(params[1].getObject());
    
    o->out->openPort(n);
}

%word closeout 1 (port --) wait 0.3s, then close an output port
{
    Out *o = (Out *)(params[0].getObject());
    usleep(300000L); // sleep a tiny bit
    o->close();
}

%word getouts 1 (port -- list) get list of possible ports for an output
{
    res->setList();
    Out *o = (Out *)(params[0].getObject());
    int ports = o->out->getPortCount();
    for(int i=0;i<ports;i++){
        PluginValue *pv = new PluginValue(o->out->getPortName(i).c_str());
        res->addToList(pv);
    }
}

%word on 4 (note vel chan port --)
{
    int note = params[0].getInt();
    int vel = params[1].getInt();
    int chan = params[2].getInt();
    Out *o = (Out *)(params[3].getObject());
    
    std::vector<unsigned char> vec(3);
    vec[0]=144+chan;
    vec[1]=note;
    vec[2]=vel;
    fflush(stdout);
    o->out->sendMessage(&vec);
}

%word off 3 (note chan port --)
{
    Out *o = (Out *)(params[2].getObject());
    std::vector<unsigned char> vec(2);
    vec[0]=128 + params[1].getInt();
    vec[1]=params[0].getInt();
    
    
    o->out->sendMessage(&vec);
}

%init
{
    api = interface;
    printf("Initialising Midi plugin, %s %s\n",__DATE__,__TIME__);
}
          
