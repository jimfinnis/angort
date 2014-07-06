/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include "../../include/plugins.h"
//#include <angort/plugins.h>

%plugin sdl

static AngortPluginInterface *api;
static SDL_Surface *screen = NULL;
static uint32_t forecol;
static uint32_t backcol;
static bool done = false; // set when we want to quit

class Surface : public PluginObject {
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

static void chkscr(){
    if(!screen)
        throw "SDL not initialised";
}

%word close 0 (--) close down SDL
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

%word fullscreenopen 2 (x/-1 y/-1 --) init SDL and open a fullscreen hw window
{
    int w = params[0].getInt();
    int h = params[1].getInt();
    if(w<0){
        const SDL_VideoInfo *v = SDL_GetVideoInfo();
        w = v->current_w;
        h = v->current_h;
    }
    openwindow(w,h,SDL_FULLSCREEN|SDL_HWSURFACE);
}

%word open 2 (x y -- ) init SDL and open a window
{
    openwindow(params[0].getInt(),params[1].getInt(),0);
}

%word scr 0 (-- surface) get the screen surface
{
    // got to make a new one of these each time, rather
    // than keep it static, because the object will keep
    // getting deleted.
    Surface *screenSurf = new Surface(screen);
    screenSurf->isScreen=true;
    
    res->setObject(screenSurf);
}

%word width 1 (surface -- int) get width of surface
{
    Surface *so = (Surface *)(params[0].getObject());
    res->setInt(so->s->w);
}

%word height 1 (surface -- int) get width of surface
{
    Surface *so = (Surface *)(params[0].getObject());
    res->setInt(so->s->h);
}

%word load 1 (file -- surf/none) load an image into a surface
{
    chkscr();
    SDL_Surface *tmp = IMG_Load(params[0].getString());
    if(!tmp)
        res->setNone();
    else {
        Surface *s = new Surface(SDL_DisplayFormat(tmp));
        SDL_FreeSurface(tmp);
        res->setObject(s);
    }
}

%word blit 7 (dx dy sx sy sw/none sh/none surf --) blit a surface to the screen
{
    chkscr();
    Surface *so = (Surface *)(params[6].getObject());
    SDL_Surface *s = so->s;
    SDL_Rect src,dst;
    
    dst.x = params[0].getInt(); // destination x
    dst.y = params[1].getInt(); // destination y
    dst.w = 0;
    dst.h = 0;
    src.x = params[2].getInt(); // source x
    src.y = params[3].getInt(); // source y
    src.h = params[4].type == PV_NONE ? params[4].getInt() : s->w; // source width / none if all
    src.w = params[5].type == PV_NONE ? params[5].getInt() : s->h; // source height / none if all
    
    SDL_BlitSurface(s,&src,screen,&dst);
}

%word flip 0 (--) flip front and back buffer
{
    chkscr();
    SDL_Flip(screen);
}

%word col 3 (r g b --) set colour 
{
    chkscr();
    int colr = params[0].getInt();
    int colg = params[1].getInt();
    int colb = params[2].getInt();
    forecol = SDL_MapRGB(screen->format,colr,colg,colb);
}
%word bcol 3 (r g b --) set back colour 
{
    chkscr();
    int colr = params[0].getInt();
    int colg = params[1].getInt();
    int colb = params[2].getInt();
    backcol = SDL_MapRGB(screen->format,colr,colg,colb);
}

%word clear 0 (--) clear the screen to the background colour
{
    chkscr();
    SDL_FillRect(screen,NULL,backcol);
}

%word fillrect 4 (x y w h --) draw a filled rectangle in current colour
{
    chkscr();
    SDL_Rect r;
    
    r.x = params[0].getInt();
    r.y = params[1].getInt();
    r.w = params[2].getInt();
    r.h = params[3].getInt();
    
    SDL_FillRect(screen,&r,forecol);
}

// various callbacks
Value *onKeyDown = NULL;
Value *onKeyUp = NULL;
Value *onMouse = NULL;
Value *onDraw = NULL;

%word ondraw 1 (callable --) set the draw callback, of spec (--)
{
    if(onDraw)
        api->releaseCallable(onDraw);
    onDraw = params[0].getCallable();
}

%word onkeyup 1 (callable --) set the key up callback, of spec (keysym --)
{
    if(onKeyUp)
        api->releaseCallable(onKeyUp);
    onKeyUp = params[0].getCallable();
}

%word onkeydown 1 (callable --) set the key down callback, of spec (keysym --)
{
    if(onKeyDown)
        api->releaseCallable(onKeyDown);
    onKeyDown = params[0].getCallable();
}

%word onmouse 1 (callable --) set the draw callback, of spec (--)
{
    if(onMouse)
        api->releaseCallable(onMouse);
    onMouse = params[0].getCallable();
}



%word loop 0 (--) start the main game loop
{
    chkscr();
    PluginValue pv; // used to pass data to callbacks
    while(!done){
        SDL_Event e;
        while(SDL_PollEvent(&e)){
            switch(e.type){
            case SDL_QUIT:
                done=true;break;
            case SDL_KEYDOWN:
                if(onKeyDown){
                    pv.setInt(e.key.keysym.sym);
                    api->call(onKeyDown,&pv);
                }
                break;
            case SDL_KEYUP:
                if(onKeyUp){
                    pv.setInt(e.key.keysym.sym);
                    api->call(onKeyUp,&pv);
                }
                break;
            default:break;
            }
        }
        if(onDraw)
            api->call(onDraw);
    }
}

%word done 0 (--) set the done flag to end the main loop
{
    done=true;
}


%init
{
    api=interface;
    printf("Initialising SDL plugin, %s %s\n",__DATE__,__TIME__);
    SDL_Init(SDL_INIT_EVERYTHING);
}
