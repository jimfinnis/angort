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

static AngortPluginInterface *api;

class Out : public PluginObject {
public:
    RtMidiOut *out;
    
    Out(){
        out = new RtMidiOut();
    }
    
    ~Out(){
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


%word getouts 1 (port -- list) get list of ports for an output
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
    Out *o = (Out *)(params[3].getObject());
    std::vector<unsigned char> vec;
    vec[0]=144 + params[2].getInt();
    vec[1]=params[0].getInt();
    vec[2]=params[1].getInt();
    o->out->sendMessage(&vec);
}

%word off 3 (note chan port --)
{
    Out *o = (Out *)(params[2].getObject());
    std::vector<unsigned char> vec;
    vec[0]=128 + params[1].getInt();
    vec[1]=params[0].getInt();
    o->out->sendMessage(&vec);
}

%init
{
    api = interface;
    printf("Initialising Midi plugin, %s %s\n",__DATE__,__TIME__);
}
          
