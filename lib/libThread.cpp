/**
 * @file thread.cpp
 * @brief  Brief description of file.
 *
 */

#include <pthread.h>
#include "angort.h"
#include "wrappers.h"

using namespace angort;

class MsgBuffer {
    static const int size=8;
    Value msgs[size];
    int wr,rd;
    int length;
    pthread_mutex_t mutex,mutex2;
    pthread_cond_t cond,cond2;
    
    bool isEmpty(){
        return length==0;
    }
    
    bool isFull(){
        return length==size;
    }
    
    void _read(Value *v){
        if(isEmpty())printf("NO MESSAGE\n");
        msgs[rd].t->clone(v,msgs+rd);
//        v->copy(msgs+rd);
        rd++;
        rd %= size;
        length--;
    }
    
    bool _write(Value *v){
        if(isFull())return false;
        wr++;
        wr %= size;
        //        msgs[wr].copy(v);
        v->t->clone(msgs+wr,v);
        length++;
        return true;
    }
public:
    MsgBuffer(){
        wr=size-1;
        rd=0;
        length=0;
        pthread_mutex_init(&mutex,NULL);
        pthread_cond_init(&cond,NULL);
        pthread_mutex_init(&mutex2,NULL);
        pthread_cond_init(&cond2,NULL);
    }
    ~MsgBuffer(){
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
        pthread_mutex_destroy(&mutex2);
        pthread_cond_destroy(&cond2);
    }
    
    // wait for a message, blocking if we have to
    void waitAndRead(Value *dest){
        pthread_mutex_lock(&mutex);
        while(isEmpty())
            pthread_cond_wait(&cond,&mutex);
        _read(dest);
        pthread_cond_signal(&cond2);
//        printf("%d SIGNALLING.\n",snark);
        pthread_mutex_unlock(&mutex);
//        printf("%d read ok\n",snark);
    }
    
    // write a message, return false if no room
    bool writeNoWait(Value *v){
        bool ok;
        pthread_mutex_lock(&mutex);
        ok = _write(v);
        if(ok)pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
        return ok;
    }
    
    // write a message, blocking if we're full.
    void writeWait(Value *v){
//        printf("%d write \n",snark);
        pthread_mutex_lock(&mutex2);
        while(isFull()){
//            printf("%d FULL.\n",snark);
            pthread_cond_wait(&cond2,&mutex2);
        }
        pthread_mutex_lock(&mutex);
        _write(v);
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
//        printf("%d write ok\n",snark);
        pthread_mutex_unlock(&mutex2);
    }
    
    
    
};

void *_threadfunc(void *);

namespace angort {
class Thread : public GarbageCollected {
    // our run environment
    Runtime *runtime;
    
public:
    Value func;
    Value retval; // the returned value
    Value arg;
    pthread_t thread;
    MsgBuffer msgs;
    int id;
    
    bool isRunning(){
        return runtime != NULL;
    }
    Thread(Angort *ang,Value *v,Value *_arg) : GarbageCollected("thread"){
        WriteLock lock=WL(&globalLock);
        incRefCt(); // make sure we don't get deleted until complete
        func.copy(v);
        _arg->t->clone(&arg,_arg); // clone the argument
//        printf("%p %p\n",arg.v.gc,_arg->v.gc);
        runtime = new Runtime(ang,"<thread>");
        id = runtime->id;
        runtime->thread = this;
        runtime->pushval()->copy(&arg);
//            printf("Start of run refcount is %d\n",(int)refct);
        pthread_create(&thread,NULL,_threadfunc,this);
//        printf("Created thread %d[%lu] at %p/%s runtime at %p\n",id,thread,this,func.t->name,runtime);
    }
    ~Thread(){
        if(runtime)throw WTF;
//        printf("Thread %d destroyed at %p\n",id,this);
    }
    void run(){
        try {
            runtime->runValue(&func);
        } catch(Exception e){
            WriteLock lock=WL(&globalLock);
            printf("Exception in thread %d\n",runtime->id);
        }
        
        {
            WriteLock lock=WL(&globalLock);
            // pop the last value off the thread runtime
            // and copy it into the return value, if there is one.
            if(!runtime->stack.isempty()){
                retval.copy(runtime->stack.popptr());
            }
        }
        //        printf("Deleting runtime of thread %d at %p\n",id,runtime);
        delete runtime;
        //        printf("Deleting OK\n");
        runtime = NULL;
        func.clr();
        // decrement refct, and delete this if it's zero. This is kind
        // of OK, here - it's the last thing that happens.
        //            printf("End of run refcount is %d\n",(int)refct);
        if(decRefCt()){
            printf("Refcount delete at end of run()\n");
            delete this; 
        }
    }
};
}

void *_threadfunc(void *p){
    Thread *t = (Thread *)p;
    //    printf("starting thread at %p/%s\n",t,t->func.t->name);
    t->run();
    //    printf("Thread func terminated OK?\n");
    return NULL;
}

%name thread

// crude hack - this is the message buffer for the default thread.
static MsgBuffer defaultMsgs;

class ThreadType : public GCType {
public:
    ThreadType(){
        add("thread","THRD");
    }
    
    Thread *get(Value *v){
        if(v->t!=this)
            throw RUNT(EX_TYPE,"not a thread");
        return (Thread *)(v->v.gc);
    }
    
    // create a new thread!
    void set(Value *v,Angort *ang,Value *func,Value *pass){
        if(func->t != Types::tCode)
            throw RUNT(EX_TYPE,"").set("not a codeblock, is %s (can't be a closure)",func->t->name);
        v->clr();
        v->t=this;
        v->v.gc = new Thread(ang,func,pass);
        incRef(v);
    }
    
    // set a value to a thread
    void set(Value *v, Thread *t){
        v->clr();
        v->t = this;
        v->v.gc = t;
        incRef(v);
    }
};

static ThreadType tThread;

// mutex wrapper class
struct Mutex {
    pthread_mutex_t m;
    Mutex(){
        // make the mutex recursive
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&m,&attr);
    }
    ~Mutex(){
        pthread_mutex_destroy(&m);
    }
};


// the wrapper has a wrapper type..
static WrapperType<pthread_mutex_t> tMutex("MUTX");

%type mutex tMutex pthread_mutex_t
%type thread tThread Thread


// words

%wordargs create vc (arg func --) start a new thread
Start a new thread, as a function which takes an argument. The
argument is shallow-cloned before being pushed onto the thread function's
stack. If the thread finishes with a non-empty stack, the top value can
be retrieved with thread$retval. 
{
    Value v,p;
    p.copy(p0);
    v.copy(p1);
    tThread.set(a->pushval(),a->ang,&v,&p);
}

%wordargs join l (threadlist --) wait for threads to complete
{
    ArrayListIterator<Value> iter(p0);
//    printf("JOIN START\n");
    // check types first
    for(iter.first();!iter.isDone();iter.next()){
        Value *p = iter.current();
        if(p->t != &tThread)
            throw RUNT(EX_TYPE,"expected threads only in thread list");
    }
    
    // then join each in turn.
    for(iter.first();!iter.isDone();iter.next()){
        Value *p = iter.current();
        pthread_join(tThread.get(p)->thread,NULL);
    }
//    printf("JOIN END\n");
}

%word mutex (-- mutex) create a recursive mutex
{
    pthread_mutex_t *v = new pthread_mutex_t();
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(v,&attr);
    
    tMutex.set(a->pushval(),v);
}

%wordargs lock A|mutex (mutex -- ) lock a mutex
{
    pthread_mutex_lock(p0);
}
%wordargs unlock A|mutex (mutex -- ) unlock a mutex
{
    pthread_mutex_unlock(p0);
}

%wordargs retval A|thread (thread -- val) get return of a finished thread
One way of communicating with a thread is to send data into it with
the function parameter, and receive the result (after the thread has
completed) using thread$retval. If the thread is still running an
exception will be thrown. Use thread$join to wait for thread completion.
{
    WriteLock lock=WL(&globalLock);
    if(p0->isRunning()){
        throw RUNT("ex$threadrunning","thread is still running");
    }
    
    a->pushval()->copy(&p0->retval);
}

%wordargs sendnoblock vv|thread (msg thread|none -- bool) send a message value to a thread
Send a message to a thread. The default thread is indicated by "none".
Will return a boolean indicating the send was successful. 
{
    MsgBuffer *b;
    if(p1->isNone())
        b = &defaultMsgs;
    else {
        Thread *t = tThread.get(p1);
        b = &t->msgs;
    }
    a->pushInt(b->writeNoWait(p0)?1:0);
}

%wordargs send vv|thread (msg thread|none --) send a message value to a thread
Send a message to a thread. The default thread is indicated by "none".
Will wait if the buffer is full.
{
    MsgBuffer *b;
    if(p1->isNone())
        b = &defaultMsgs;
    else {
        Thread *t = tThread.get(p1);
        b = &t->msgs;
    }
    b->writeWait(p0);
}

%word waitrecv (--- msg) blocking message read
Wait for a message to arrive on this thread and return it.
{
    MsgBuffer *b;
    if(a->thread)
        b = &a->thread->msgs;
    else
        b = &defaultMsgs;
    Value v;
    b->waitAndRead(&v);
    a->pushval()->copy(&v);
}

%word cur (-- thread|none) return current thread or none for main thread
{
    if(a->thread)
        tThread.set(a->pushval(),a->thread);
    else
        a->pushNone();
}

%wordargs id v|thread (none|thread -- id) get ID from thread, -1 for none
{
    if(p0->isNone())
        a->pushNone();
    else {
        Thread *t = tThread.get(p0);
        a->pushInt(t->id);
    }
}


%init
{
/*
    fprintf(stderr,"Initialising THREAD plugin, %s %s\n",__DATE__,__TIME__);
    
    fprintf(stderr,
            "DO NOT USE THIS to do ANYTHING with collections - \n"
            "arraylists and hashes move around in memory and this will\n"
            "royally mess things up. Sorry.\n"
            );
    fprintf(stderr,"Thread type ID = %u\n",tThread.id);
    if(!angort::hasLocking()){
        fprintf(stderr,"Cannot use the thread library - Angort was not compiled with locking.\n");
        exit(1);
   }
 */
}    

