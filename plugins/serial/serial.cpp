/**
 * @file serial.cpp
 * @brief  Brief description of file.
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>

#include <angort/angort.h>

static int fd=-100;

%name serial
%shared

using namespace angort;

class SerialBuffer : public GarbageCollected {
public:
    uint8_t *data;
    int size;
    
    SerialBuffer(int s){
        size =s;
        data = new uint8_t[s];
    }
    
    virtual ~SerialBuffer(){
        free(data);
    }
};

class SerialBufferType : public GCType {
public:
    SerialBufferType(){
        add("SerialBuffer","SBUF");
    }
    
    SerialBuffer *get(Value *v){
        if(v->t!=this)throw RUNT("not a serial buffer");
        return (SerialBuffer *)(v->v.gc);
    }
    
    void set(Value *v, SerialBuffer *s){
        v->clr();
        v->t = this;
        v->v.gc = s;
        incRef(v);
    }
};

static SerialBufferType tBuf;

    
    

%word init (baud device -- bool)
{
    Value *p[2];
    a->popParams(p,"ns");
    struct termios newtio;
    
    /*open serial port*/
    fd = open(p[1]->toString().get(), O_RDWR | O_NOCTTY );
    
    if (fd <0)
    {
        fprintf(stderr,"Error opening device\n");
        a->pushInt(0);
        return;
    }
    
    bzero(&newtio, sizeof(newtio));
    int baud = p[0]->toInt();
    switch(baud)
    {
    case 300:
        newtio.c_cflag = B300 | CS8 | CLOCAL | CREAD;
        break;
    case 1200:
        newtio.c_cflag = B1200 | CS8 | CLOCAL | CREAD;
        break;
    case 2400:
        newtio.c_cflag = B2400 | CS8 | CLOCAL | CREAD;
        break;
    case 4800:
        newtio.c_cflag = B4800 | CS8 | CLOCAL | CREAD;
        break;
    case 9600:
        newtio.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
        break;
    case 19200:
        newtio.c_cflag = B19200 | CS8 | CLOCAL | CREAD;
        break;
    case 38400:
        newtio.c_cflag = B38400 | CS8 | CLOCAL | CREAD;
        break;
    case 57600:
        newtio.c_cflag = B57600 | CS8 | CLOCAL | CREAD;
        break;
    case 115200:
        newtio.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
        break;
    default:
        fprintf(stderr,"Invalid Baud Rate %d\n",baud);
        close(fd);
        fd=-100;
        a->pushInt(0);
        return;
    }
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;
    
    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;
    
    newtio.c_cc[VTIME] = 100;   /* inter-character timer  */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 1 char received */
    /*flush out any old data*/
    tcflush(fd, TCIFLUSH);
    /*set baud rate etc*/
    tcsetattr(fd,TCSANOW,&newtio);
    //fprintf(stderr,"Opened device %s at %d bits per second\n",device,baud);
}    

%word close (--)
{
    if(fd!=-100){
        close(fd);
        fd=-100;
    }
}

%word rx (count -- status buffer) blocking binary read, status false on timeout
{
    Value *p;
    a->popParams(&p,"n");
    
    int count = p->toInt();
    SerialBuffer *b = new SerialBuffer(count);
        
    /*get the response*/
    ssize_t out;
    
    uint8_t *ptr = b->data;
    while(count>0){
        out=read(fd,ptr,count);
        count -= out;
        ptr += out;
    }
    
    a->pushInt(1);
    tBuf.set(a->pushval(),b);
}

%word tx (buffer --) binary write
{
    Value *p;
    a->popParams(&p,"A",&tBuf);
    SerialBuffer *b = tBuf.get(p);
    
    int status = write(fd,b->data,b->size);
    
    if(status != 0)
        fprintf(stderr,"Write failure\n");
}

%word mkbuf (size -- buffer)
{
    Value *p;
    a->popParams(&p,"n");
    
    int count = p->toInt();
    SerialBuffer *b = new SerialBuffer(count);
    tBuf.set(a->pushval(),b);
}

%word set (value n buffer --)
{
    Value *p[3];
    a->popParams(p,"nnA",&tBuf);
    uint8_t val = p[0]->toInt();
    int n = p[1]->toInt();
    SerialBuffer *b = tBuf.get(p[2]);
    
    if(n>=b->size || n<0)
        fprintf(stderr,"cannot write index %d in buffer of size %d\n",n,b->size);
    else
        b->data[n]=val;
}


%word get (n buffer -- value)
{
    Value *p[2];
    a->popParams(p,"nA",&tBuf);
    int n = p[0]->toInt();
    SerialBuffer *b = tBuf.get(p[1]);
    
    if(n>=b->size || n<0){
        fprintf(stderr,"cannot write index %d in buffer of size %d\n",n,b->size);
        a->pushNone();
    } else {
        a->pushInt(b->data[n]);
    }
}

%word parse (buffer format -- list)
{
    Value *params[2];
    a->popParams(params,"As",&tBuf);
    SerialBuffer *b = tBuf.get(params[0]);
    const StringBuffer& strb=params[1]->toString();
    const char *s = strb.get();
    
    ArrayList<Value> *list = Types::tList->set(a->pushval());
    
    const uint8_t *p = b->data;
    do {
        switch(*s){
        case 'b':
            Types::tInteger->set(list->append(),*p);p++;
            break;
        case 'c':
            Types::tInteger->set(list->append(),*(char *)p);p++;
            break;
        case 'i':
            Types::tInteger->set(list->append(),*(int16_t *)p);p+=2;
            break;
        case 'f':
            Types::tFloat->set(list->append(),*(float *)p);p+=4;
            break;
        }
    } while(*s++);
}

%word read (-- string) read a line of text, blocking
{
    char buf[256];
    char *ptr=buf;
    
    for(;;){
        char c;
        if(read(fd,&c,1)==1){
            if(c=='\n')
                break;
            *ptr++ = c;
        }
    }
    *ptr=0;
    a->pushString(buf);
}

%word write (string --) transmit a line of text
{
    Value *p;
    a->popParams(&p,"s");
    const StringBuffer& buf = p->toString();
    const char *s = buf.get();
    
    write(fd,s,strlen(s));
}
    

    
%init
{
    printf("Starting simple serial library\n");
}
