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
#include <angort/angort.h>

using namespace angort;

%name sdl
%shared

static SDL_Window *screen = NULL;
static SDL_Renderer *renderer = NULL;

struct Colour {
    uint8_t r,g,b,a;
    void set(){
        SDL_SetRenderDrawColor(renderer,r,g,b,a);
    }
};

Colour forecol={255,255,255,255};
Colour backcol={0,0,0,255};

static bool done = false; // set when we want to quit

class Texture : public GarbageCollected {
public:
    SDL_Texture *t;
    Texture(SDL_Texture *_t){  t=_t; }
    virtual ~Texture(){
        SDL_DestroyTexture(t);
    }
};

class TextureType : public GCType {
public:
    TextureType(){
        add("Texture","SDLS");
    }
    
    Texture *get(Value *v){
        if(v->t != this)
            throw RUNT("not a Texture");
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

static void chkscr(){
    if(!screen)
        throw RUNT("SDL not initialised");
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
    SDL_RenderFillRect(renderer,NULL);
    SDL_RenderPresent(renderer);
    SDL_RenderFillRect(renderer,NULL);
    SDL_RenderPresent(renderer);
    forecol.set();
}

%word fullscreenopen (w/-1 h/-1 --) init SDL and open a fullscreen hw window
{
    Value *p[2];
    a->popParams(p,"nn");
    
    int w = p[0]->toInt();
    int h = p[1]->toInt();
    if(w<0){
        SDL_DisplayMode disp;
        int ret = SDL_GetCurrentDisplayMode(0,&disp);
        if(ret)
            throw RUNT("").set("could not get video mode: %s",SDL_GetError());
        
        w = disp.w;
        h = disp.h;
    }
    openwindow("",w,h,SDL_WINDOW_FULLSCREEN|SDL_WINDOW_SHOWN);
}

%word open (w h -- ) init SDL and open a window
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

%word col (r g b --) set colour 
{
    Value *p[3];
    a->popParams(p,"nnn");
    
    chkscr();
    forecol.r = p[0]->toInt();
    forecol.g = p[1]->toInt();
    forecol.b = p[1]->toInt();
    forecol.a = 255;
    
}
%word bcol (r g b --) set back colour 
{
    Value *p[3];
    a->popParams(p,"nnn");
    
    chkscr();
    backcol.r = p[0]->toInt();
    backcol.g = p[1]->toInt();
    backcol.b = p[1]->toInt();
    backcol.a = 255;
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
    printf("Initialising SDL plugin, %s %s\n",__DATE__,__TIME__);
    SDL_Init(SDL_INIT_EVERYTHING);
    IMG_Init(IMG_INIT_JPG|IMG_INIT_PNG);
    printf("SDL initialised\n");
}


%shutdown
{
    printf("Closing down SDL\n");
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(screen);
    IMG_Quit();
    SDL_Quit();
}
