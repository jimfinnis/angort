/**
 * @file file.h
 * @brief  Brief description of file.
 *
 */

#ifndef __FILE_H
#define __FILE_H

#include <stdio.h>

class File {
    FILE *file;
public:
    File(const char *name,bool write){
        file = fopen(name,write?"w":"r");
        if(!file)
            throw SerialisationException("").set("cannot open file: %s",name);
    }
    
    ~File(){
        if(file)
            fclose(file);
    }
    
    
    void readBytes(void *buf,int len){
        if(!file)
            throw SerialisationException("file not open for reading");
        fread(buf,1,len,file);
    }
    void writeBytes(const void *buf,int len){
        if(!file)
            throw SerialisationException("file not open for writing");
        if(fwrite(buf,1,len,file)!=len)
            throw SerialisationException("premature end of file");
    }
    
    void writeInt(int i){
        int32_t t = (int32_t)i;
        writeBytes(&t,sizeof(t));
    }
    int readInt(){
        int32_t t;
        readBytes(&t,sizeof(t));
        return (int)t;
    }
    void write32(uint32_t t){
        writeBytes(&t,sizeof(t));
    }
    uint32_t read32(){
        uint32_t t;
        readBytes(&t,sizeof(t));
        return t;
    }
    void write16(uint16_t t){
        writeBytes(&t,sizeof(t));
    }
    uint16_t read16(){
        uint16_t t;
        readBytes(&t,sizeof(t));
        return t;
    }
    void writeFloat(float t){
        writeBytes(&t,sizeof(t));
    }
    float readFloat(){
        float t;
        readBytes(&t,sizeof(t));
        return t;
    }
    
    void writeString(const char *s){
        int len = strlen(s)+1; // note we store the null too.
        write16(len);
        writeBytes(s,len);
    }
    
    char *readString(char *s,int maxlen){
        int len = read16();
        if(len>maxlen)throw WTF;
        readBytes(s,len);
        return s;
    }
    char *readStringAlloc(){
        int len = read16();
        char *s = (char *)malloc(len);
        readBytes(s,len);
        return s;
    }
};
    
    


#endif /* __FILE_H */
