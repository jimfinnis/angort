/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <angort/angort.h>

using namespace angort;

#include "texture.h"
#include "font.h"

%name sdl
%shared

static SDL_Window *screen = NULL;
static SDL_Renderer *renderer = NULL;
static bool inited=false;


static bool done = false; // set when we want to quit


class ColProperty : public Property {
public:
    SDL_Colour col;
    ColProperty(uint8_t r,uint8_t g,uint8_t b,uint8_t a){
        col.r = r;
        col.g = g;
        col.b = b;
        col.a = a;
    }
    
    virtual void postSet(){
        ArrayList<Value> *list = Types::tList->get(&v);
        col.r = list->get(0)->toInt();
        col.g = list->get(1)->toInt();
        col.b = list->get(2)->toInt();
        col.a = list->get(3)->toInt();
    }
    
    virtual void preGet(){
        ArrayList<Value> *list = Types::tList->set(&v);
        Types::tInteger->set(list->append(),col.r);
        Types::tInteger->set(list->append(),col.g);
        Types::tInteger->set(list->append(),col.b);
        Types::tInteger->set(list->append(),col.a);
    }
    
    void set(){
        SDL_SetRenderDrawColor(renderer,col.r,col.g,col.b,col.a);
    }
    
};

ColProperty forecol(255,255,255,255);
ColProperty backcol(0,0,0,255);


static void chkscr(){
    if(!inited)
        throw RUNT("SDL not initialised");
    if(!screen)
        throw RUNT("SDL screen not open");
}

%word close (--) close SDL window
{
    if(screen)
        SDL_Quit();
    screen=NULL;
}

static void openwindow(const char *title, int w,int h,int flags){
    // must be 24-bit
    
    screen = SDL_CreateWindow(title,
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              w,h,flags);
    if(!screen)
        throw RUNT("cannot open screen");
    renderer = SDL_CreateRenderer(screen,-1,0);
    
    backcol.set();
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
    SDL_RenderFillRect(renderer,NULL);
    SDL_RenderPresent(renderer);
    SDL_RenderFillRect(renderer,NULL);
    SDL_RenderPresent(renderer);
    forecol.set();
}

%word fullscreenopen (w/none h/none --) init SDL and open a fullscreen hw window
{
    Value *p[2];
    a->popParams(p,"NN");
    
    int w;
    int h;
    if(p[0]->isNone()){
        SDL_DisplayMode disp;
        int ret = SDL_GetCurrentDisplayMode(0,&disp);
        if(ret)
            throw RUNT("").set("could not get video mode: %s",SDL_GetError());
        
        w = disp.w;
        h = disp.h;
    } else {
        w = p[0]->toInt();
        h = p[0]->toInt();
    }
        
    openwindow("",w,h,SDL_WINDOW_FULLSCREEN|SDL_WINDOW_SHOWN);
}

%word open (title w h -- ) init SDL and open a window
{
    Value *p[3];
    a->popParams(p,"snn");
    
    int w = p[1]->toInt();
    int h = p[2]->toInt();
    
    openwindow(p[0]->toString().get(),w,h,SDL_WINDOW_SHOWN);
}

%word scrsize (-- height width) get screen dimensions
{
    int w,h;
    SDL_GetWindowSize(screen,&w,&h);
    a->pushInt(h);
    a->pushInt(w);
}
    

%word texsize (t -- height width) get texture dimensions
{
    Value *p;
    a->popParams(&p,"a",&tTexture);
    
    int w,h;
    SDL_QueryTexture(tTexture.get(p)->t,NULL,NULL,&w,&h);
    a->pushInt(h);
    a->pushInt(w);
}

%word load (file -- surf/none) load an image into a texture
{
    Value *p;
    a->popParams(&p,"s");
    
    chkscr(); // need a screen open to do format conversion
    
//    printf("attempting load: %s\n",p->toString().get());
    SDL_Surface *tmp = IMG_Load(p->toString().get());
    if(!tmp)
        printf("Failed to load %s\n",p->toString().get());
    
    p = a->pushval();
    
    if(!tmp)
        p->setNone();
    else {
        SDL_Texture *t = SDL_CreateTextureFromSurface(renderer,tmp);
        if(!t){
            printf("Failed to create texture %s: %s\n",p->toString().get(),
                   SDL_GetError());
            p->setNone();
            SDL_FreeSurface(tmp);
            return;
        }
        if(SDL_SetTextureBlendMode(t,SDL_BLENDMODE_BLEND))
            printf("blend mode not supported\n");
        Texture *tt = new Texture(t);
        tTexture.set(p,tt);
        SDL_FreeSurface(tmp);
    }
}

%word blit (dx dy dw/none dh/none surf --) basic texture blit
{
    Value *p[5];
    a->popParams(p,"nnNNa",&tTexture);
    
    chkscr();
    SDL_Texture *t = tTexture.get(p[4])->t;
    SDL_Rect src,dst;
    
    int w,h;
    SDL_QueryTexture(t,NULL,NULL,&w,&h);
    
    dst.x = p[0]->toInt(); // destination x
    dst.y = p[1]->toInt(); // destination y
    dst.w = p[2]->isNone() ? w : p[2]->toInt(); // dest width
    dst.h = p[3]->isNone() ? h : p[3]->toInt(); // dest height
    src.x = 0;
    src.y = 0;
    src.w = w;
    src.h = h;
    
    if(SDL_RenderCopy(renderer,t,&src,&dst)<0){
        printf("blit error: %s\n",SDL_GetError());
    }
}

%word exblit (dx dy dw/none dh/none sx sy sw/none sh/none surf --) blit a texture to the screen
{
    Value *p[9];
    a->popParams(p,"nnNNnnNNa",&tTexture);
    
    chkscr();
    SDL_Texture *t = tTexture.get(p[8])->t;
    SDL_Rect src,dst;
    
    int w,h;
    SDL_QueryTexture(t,NULL,NULL,&w,&h);
    
    dst.x = p[0]->toInt(); // destination x
    dst.y = p[1]->toInt(); // destination y
    dst.w = p[2]->isNone() ? w : p[2]->toInt(); // dest width
    dst.h = p[3]->isNone() ? h : p[3]->toInt(); // dest height
    src.x = p[4]->toInt(); // source x
    src.y = p[5]->toInt(); // source y
    src.w = p[6]->isNone() ? w : p[6]->toInt(); // source width
    src.h = p[7]->isNone() ? h : p[7]->toInt(); // source height
    
    if(SDL_RenderCopy(renderer,t,&src,&dst)<0){
        printf("blit error: %s\n",SDL_GetError());
    }
}

%word flip (--) flip front and back buffer
{
    chkscr();
    SDL_RenderPresent(renderer);
}


%word clear (--) clear the screen to the background colour
{
    chkscr();
    backcol.set();
    SDL_RenderFillRect(renderer,NULL);
}

%word fillrect (x y w h --) draw a filled rectangle in current colour
{
    Value *p[4];
    a->popParams(p,"nnnn");
    
    chkscr();
    SDL_Rect r;
    
    r.x = p[0]->toInt();
    r.y = p[1]->toInt();
    r.w = p[2]->toInt();
    r.h = p[3]->toInt();
    
    forecol.set();
    SDL_RenderFillRect(renderer,&r);
}

%word line (x1 y1 x2 y2 --) draw a line in current colour
{
    Value *p[4];
    a->popParams(p,"nnnn");
    
    chkscr();
    
    forecol.set();
    SDL_RenderDrawLine(renderer,p[0]->toInt(),
                       p[1]->toInt(),
                       p[2]->toInt(),
                       p[3]->toInt());
}
    


%word openfont (file size -- font) open a TTF font
{
    Value *p[2];
    a->popParams(p,"sn");
    chkscr();
    
    Value *v = a->pushval();
    TTF_Font *f = TTF_OpenFont(p[0]->toString().get(),p[1]->toInt());
    if(!f){
        printf("Failed to load font %s\n",p[0]->toString().get());
        v->setNone();
    } else {
        Font *fo = new Font(f);
        tFont.set(v,fo);
    }
}

%word maketext (text font -- texture) draw text to a texture
{
    Value *p[2];
    a->popParams(p,"sa",&tFont);
    chkscr();
    
    SDL_Surface *tmp = TTF_RenderUTF8_Blended(tFont.get(p[1])->f,
                                             p[0]->toString().get(),
                                             forecol.col);
    
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer,tmp);
    if(SDL_SetTextureBlendMode(tex,SDL_BLENDMODE_BLEND))
        printf("blend mode not supported\n");
    SDL_FreeSurface(tmp);
    
    Value *v = a->pushval();
    tTexture.set(v,new Texture(tex));
}

%word fontsize (text font -- h w) get rendered size
{
    Value *p[2];
    a->popParams(p,"sa",&tFont);
    chkscr();
    
    int w,h;
    if(TTF_SizeUTF8(tFont.get(p[1])->f,p[0]->toString().get(),&w,&h)){
        printf("Error getting size of string\n");
        a->pushNone();
        a->pushNone();
    }else{
        a->pushInt(h);
        a->pushInt(w);
    }
}


// various callbacks, all initially "none"
Value onKeyDown;
Value onKeyUp;
Value onMouseMove,onMouseUp,onMouseDown;
Value onDraw;

%word ondraw (callable --) set the draw callback, of spec (--)
{
    Value *p;
    a->popParams(&p,"c");
    onDraw.copy(p);
}

%word onkeyup (callable --) set the key up callback, of spec (keysym --)
{
    Value *p;
    a->popParams(&p,"c");
    onKeyUp.copy(p);
}

%word onkeydown (callable --) set the key down callback, of spec (keysym --)
{
    Value *p;
    a->popParams(&p,"c");
    onKeyDown.copy(p);
}

%word onmousemove (callable --) set the mouse motion callback, of spec (x y)
{
    Value *p;
    a->popParams(&p,"c");
    onMouseMove.copy(p);
}
%word onmousedown (callable --) set the mouse down callback, of spec (x y button--)
{
    Value *p;
    a->popParams(&p,"c");
    onMouseDown.copy(p);
}
%word onmouseup (callable --) set the mouse up callback, of spec (x y button--)
{
    Value *p;
    a->popParams(&p,"c");
    onMouseUp.copy(p);
}

%word loop (--) start the main game loop
{
    chkscr();
    while(!done){
        SDL_Event e;
        while(SDL_PollEvent(&e)){
            switch(e.type){
            case SDL_QUIT:
                done=true;break;
            case SDL_KEYDOWN:
                if(!onKeyDown.isNone()){
                    a->pushInt(e.key.keysym.sym);
                    a->runValue(&onKeyDown);
                }
                break;
            case SDL_KEYUP:
                if(!onKeyUp.isNone()){
                    a->pushInt(e.key.keysym.sym);
                    a->runValue(&onKeyUp);
                }
                break;
            case SDL_MOUSEMOTION:
                if(!onMouseMove.isNone()){
                    a->pushInt(e.motion.x);
                    a->pushInt(e.motion.y);
                    a->runValue(&onMouseMove);
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                if(!onMouseDown.isNone()){
                    a->pushInt(e.motion.x);
                    a->pushInt(e.motion.y);
                    a->pushInt(e.button.button);
                    a->runValue(&onMouseDown);
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if(!onMouseUp.isNone()){
                    a->pushInt(e.motion.x);
                    a->pushInt(e.motion.y);
                    a->pushInt(e.button.button);
                    a->runValue(&onMouseUp);
                }
                break;
            default:break;
            }
        }
        if(!onDraw.isNone())
            a->runValue(&onDraw);
    }
    done = false; // reset the done flag
}

%word done (--) set the done flag to end the main loop
{
    done=true;
}


%init
{
    fprintf(stderr,"Initialising SDL plugin, %s %s\n",__DATE__,__TIME__);
    SDL_Init(SDL_INIT_EVERYTHING);
    IMG_Init(IMG_INIT_JPG|IMG_INIT_PNG);
    TTF_Init();
    fprintf(stderr,"SDL initialised\n");
    inited=true;
    
    a->registerProperty("col",&forecol,"sdl");
    a->registerProperty("bcol",&backcol,"sdl");
    a->registerProperty("tcol",new TextureColProperty(a),"sdl");
    a->registerProperty("talpha",new TextureAlphaProperty(a),"sdl");
}


%shutdown
{
    fprintf(stderr,"Closing down SDL\n");
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(screen);
    inited=false;
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}
