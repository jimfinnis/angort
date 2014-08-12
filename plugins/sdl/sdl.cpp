/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <angort/angort.h>

using namespace angort;

%name sdl
%shared

static SDL_Surface *screen = NULL;
static uint32_t forecol;
static uint32_t backcol;
static bool done = false; // set when we want to quit


class Surface : public GarbageCollected {
public:
    SDL_Surface *s;
    bool isScreen;
    
    Surface(SDL_Surface *_s){
        s=_s;
    }
    
    virtual ~Surface(){
        if(!isScreen)
            SDL_FreeSurface(s);
    }
};

class SurfaceType : public GCType {
public:
    SurfaceType(){
        add("surface","SDLS");
    }
    
    Surface *get(Value *v){
        if(v->t != this)
            throw RUNT("not a surface");
        return (Surface *)(v->v.gc);
    }
    
    void set(Value *v, Surface *s){
        v->clr();
        v->t = this;
        v->v.gc = s;
        incRef(v);
    }
};

static SurfaceType tSurface;

static void chkscr(){
    if(!screen)
        throw "SDL not initialised";
}

%word close (--) close down SDL
{
    if(screen)
        SDL_Quit();
    screen=NULL;
}

static void openwindow(int w,int h,int flags){
    // must be 24-bit
    screen = SDL_SetVideoMode(w,h,
                              24,flags);//SDL_NOFRAME);
    if(!screen)
        throw "cannot open screen";
    
    forecol = SDL_MapRGB(screen->format,255,255,255);
    backcol = SDL_MapRGB(screen->format,0,0,0);
    SDL_FillRect(screen,NULL,backcol);
    SDL_Flip(screen);
    SDL_FillRect(screen,NULL,backcol);
    SDL_Flip(screen);
}

%word fullscreenopen (w/-1 h/-1 --) init SDL and open a fullscreen hw window
{
    Value *p[2];
    a->popParams(p,"nn");
    
    int w = p[0]->toInt();
    int h = p[1]->toInt();
    if(w<0){
        const SDL_VideoInfo *v = SDL_GetVideoInfo();
        w = v->current_w;
        h = v->current_h;
    }
    openwindow(w,h,SDL_FULLSCREEN|SDL_HWSURFACE);
}

%word open (w h -- ) init SDL and open a window
{
    Value *p[2];
    a->popParams(p,"nn");
    
    int w = p[0]->toInt();
    int h = p[1]->toInt();
    
    openwindow(w,h,0);
}

%word scr (-- surface) get the screen surface
{
    // got to make a new one of these each time, rather
    // than keep it static, because the object will keep
    // getting deleted.
    Surface *screenSurf = new Surface(screen);
    screenSurf->isScreen=true;
    
    tSurface.set(a->pushval(),screenSurf);
}

%word width (surface -- int) get width of surface
{
    Value *p;
    a->popParams(&p,"a",&tSurface);
    
    Surface *so = tSurface.get(p);
    a->pushInt(so->s->w);
}

%word height (surface -- int) get width of surface
{
    Value *p;
    a->popParams(&p,"a",&tSurface);
    
    Surface *so = tSurface.get(p);
    a->pushInt(so->s->h);
}

%word load (file -- surf/none) load an image into a surface
{
    Value *p;
    a->popParams(&p,"s");
    chkscr();
    
    printf("attempting load: %s\n",p->toString().get());
    SDL_Surface *tmp = IMG_Load(p->toString().get());
    printf("Load OK\n");
    p = a->pushval();
    
    if(!tmp)
        p->setNone();
    else {
        Surface *s = new Surface(SDL_DisplayFormat(tmp));
        tSurface.set(p,s);
        SDL_FreeSurface(tmp);
    }
}

%word blit (dx dy sx sy sw/none sh/none surf --) blit a surface to the screen
{
    Value *p[7];
    a->popParams(p,"nnnnAAb",Types::tInteger,&tSurface);
    
    chkscr();
    Surface *so = tSurface.get(p[6]);
    SDL_Surface *s = so->s;
    SDL_Rect src,dst;
    
    dst.x = p[0]->toInt(); // destination x
    dst.y = p[1]->toInt(); // destination y
    dst.w = 0;
    dst.h = 0;
    src.x = p[2]->toInt(); // source x
    src.y = p[3]->toInt(); // source y
    src.w = p[4]->isNone() ? s->w : p[4]->toInt(); // source width
    src.h = p[5]->isNone() ? s->h : p[5]->toInt(); // source height
    
    if(SDL_BlitSurface(s,&src,screen,&dst)<0){
        printf("blit error: %s\n",SDL_GetError());
    }
}

%word flip (--) flip front and back buffer
{
    chkscr();
    SDL_Flip(screen);
}

%word col (r g b --) set colour 
{
    Value *p[3];
    a->popParams(p,"nnn");
    
    chkscr();
    int colr = p[0]->toInt();
    int colg = p[1]->toInt();
    int colb = p[2]->toInt();
    forecol = SDL_MapRGB(screen->format,colr,colg,colb);
}
%word bcol (r g b --) set back colour 
{
    Value *p[3];
    a->popParams(p,"nnn");
    
    chkscr();
    int colr = p[0]->toInt();
    int colg = p[1]->toInt();
    int colb = p[2]->toInt();
    backcol = SDL_MapRGB(screen->format,colr,colg,colb);
}

%word clear (--) clear the screen to the background colour
{
    chkscr();
    SDL_FillRect(screen,NULL,backcol);
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
    
    SDL_FillRect(screen,&r,forecol);
}

// various callbacks, all initially "none"
Value onKeyDown;
Value onKeyUp;
Value onMouse;
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

%word onmouse (callable --) set the draw callback, of spec (--)
{
    Value *p;
    a->popParams(&p,"c");
    onMouse.copy(p);
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
    printf("SDL itself init.\n");
    IMG_Init(IMG_INIT_JPG|IMG_INIT_PNG);
    printf("SDL initialised\n");
}
