/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#ifndef __MAP_H
#define __MAP_H

#include "iterator.h"

/// can't be bothered to write a proper string->item dictionary 
/// for this project, so this is just a linked list.

template <class T> struct StringMapEnt {
    StringMapEnt(const char *s,T item){
        key = strdup(s);
        value = item;
    }
    
    ~StringMapEnt(){
        free((void *)key);
    }
    
    const char *key;
    T value;
    StringMapEnt *next;
};

template <class T> class StringMapIterator;

template <class T> class StringMap {
    friend class StringMapIterator<T>;
    int size;
private:
    StringMapEnt<T> *head;
    
    StringMapEnt<T>* findent(const char *key){
        StringMapEnt<T> *p;
        for(p=head;p;p=p->next){
            if(!strcmp(p->key,key))
                return p;
        }
        return NULL;
    }
    StringMapEnt<T>* findentreverse(T value){
        StringMapEnt<T> *p;
        for(p=head;p;p=p->next){
            if(p->value == value)
                return p;
        }
        return NULL;
    }
    
    StringMapEnt<T> *foundEnt;
        
public:
    StringMap(){
        head=NULL;
        foundEnt=NULL;
        size=0;
    }
    
    // useful for when you have primitives in here, so a NULL return
    // isn't helpful
    bool find(const char *key){
        foundEnt = findent(key);
        return foundEnt?true:false;
    }
    
    T found(){
        if(foundEnt)
            return foundEnt->value;
        else
            return 0;
    }
    
    
    T get(const char *key){
        StringMapEnt<T> *p = findent(key);
        if(p)
            return p->value;
        else
            return 0;
    }
    
    /// reverse lookup
    const char *getKey(T value){
        StringMapEnt<T> *p = findentreverse(value);
        if(p)
            return p->key;
        else
            return NULL;
    }
    
    /// set, will overwrite
    void set(const char *key,T value){
        StringMapEnt<T> *p;
        if(p=findent(key)){
            p->value = value;
        } else {
            p = new StringMapEnt<T>(key,value);
            p->next = head;
            head = p;
        }
        size++;
    }
    
    void clear(){
        StringMapEnt<T> *p,*q;
        for(p=head;p;p=q){
            q=p->next;
            delete p;
        }
        head=NULL;
        size=0;
    }
    
    void listKeys();
    int count(){
        return size;
    }
};

template <class T> class StringMapIterator : public Iterator<StringMapEnt<T>* > {
public:
    
    StringMapIterator(StringMap<T> *m){
        map = m;
    }
    
    virtual void first(){
        ptr = map->head;
    }
    
    virtual void next(){
        ptr = ptr->next;
    }
    
    virtual bool isDone() const{
        return ptr == NULL;
    }
    
    virtual StringMapEnt<T> *current(){
        return ptr;
    }
        
private:
    StringMap<T> *map;
    StringMapEnt<T> *ptr;
    
};

template<class T> void StringMap<T>::listKeys(){
    StringMapIterator<T> iter(this);
    
    int lengthSoFar=-1;
    for(iter.first();!iter.isDone();iter.next()){
        if(lengthSoFar<0){
            printf("    ");lengthSoFar++;
        } else if(lengthSoFar>50){
            puts("");
            printf("    ");
            lengthSoFar=0;
        }
        const char *s = iter.current()->key;
        lengthSoFar+=strlen(s);
        printf("%s ",s,lengthSoFar);
    }
    if(lengthSoFar>0)puts("");
}


#endif /* __MAP_H */
