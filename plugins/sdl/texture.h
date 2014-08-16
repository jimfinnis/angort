/**
 * @file texture.h
 * @brief  Brief description of file.
 *
 */

#ifndef __TEXTURE_H
#define __TEXTURE_H

class Texture : public GarbageCollected {
public:
    SDL_Texture *t;
    Texture(SDL_Texture *_t){ 
        t=_t;
    }
    virtual ~Texture(){
//        printf("Destroying texture\n");
        SDL_DestroyTexture(t);
    }
};

class TextureType : public GCType {
public:
    TextureType(){
        add("texture","SDLT");
    }
    
    Texture *get(Value *v){
        if(v->t != this)
            throw RUNT("not a texture");
        return (Texture *)(v->v.gc);
    }
    
    void set(Value *v, Texture *s){
        v->clr();
        v->t = this;
        v->v.gc = s;
        incRef(v);
    }
};

static TextureType tTexture;


class TextureColProperty : public Property {
private:
    SDL_Texture *tex;
    Angort *a;
    
public:
    TextureColProperty(Angort *_a){
        a= _a;
    }
    
    
    void setTex(){
        Value *p;
        a->popParams(&p,"a",&tTexture);
        tex = tTexture.get(p)->t;
    }
    
    virtual void preSet(){
        setTex();
    }
    
    virtual void postSet(){
        ArrayList<Value> *list = Types::tList->get(&v);
        int r,g,b;
        r = list->get(0)->toInt();
        g = list->get(1)->toInt();
        b = list->get(2)->toInt();
        
        SDL_SetTextureColorMod(tex,r,g,b);
    }
    
    virtual void preGet(){
        setTex();
        
        uint8_t r,g,b;
        SDL_GetTextureColorMod(tex,&r,&g,&b);
        
        ArrayList<Value> *list = Types::tList->set(&v);
        Types::tInteger->set(list->append(),r);
        Types::tInteger->set(list->append(),g);
        Types::tInteger->set(list->append(),b);
    }
};

class TextureAlphaProperty : public Property {
private:
    SDL_Texture *tex;
    Angort *a;
    
public:
    TextureAlphaProperty(Angort *_a){
        a= _a;
    }
    
    
    void setTex(){
        Value *p;
        a->popParams(&p,"a",&tTexture);
        tex = tTexture.get(p)->t;
    }
    
    virtual void preSet(){
        setTex();
    }
    
    virtual void postSet(){
        SDL_SetTextureAlphaMod(tex,v.toInt());
    }
    
    virtual void preGet(){
        setTex();
        uint8_t alp;
        SDL_GetTextureAlphaMod(tex,&alp);
        a->pushInt(alp);
    }
};

#endif /* __TEXTURE_H */
