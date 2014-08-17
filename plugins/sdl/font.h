/**
 * @file font.h
 * @brief  Brief description of file.
 *
 */

#ifndef __FONT_H
#define __FONT_H

class Font : public GarbageCollected {
public:
    TTF_Font *f;
    Font(TTF_Font *_t){ 
        f=_t;
    }
    virtual ~Font(){
//        printf("Destroying font\n");
        TTF_CloseFont(f);
    }
};

class FontType : public GCType {
public:
    FontType(){
        add("font","SDLF");
    }
    
    Font *get(Value *v){
        if(v->t != this)
            throw RUNT("not a font");
        return (Font *)(v->v.gc);
    }
    
    void set(Value *v, Font *s){
        v->clr();
        v->t = this;
        v->v.gc = s;
        incRef(v);
    }
};

static FontType tFont;


#endif /* __FONT_H */
