/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <angort/plugins.h>

%plugin sdl

static SDL_Surface *screen = NULL;
static uint8_t colr=255,colg=255,colb=255;

class Surface : public PluginObject {
public:
    SDL_Surface *s;
    
    Surface(SDL_Surface *_s){
        s=_s;
    }
};

static void chkscr(){
    if(!screen)
        throw "SDL not initialised";
}

%word quit 0 (--) close down SDL
{
    if(screen)
        SDL_Quit();
    screen=NULL;
}

%word init 2 (x y -- ) init SDL and open a window
{
    // must be 24-bit
    screen = SDL_SetVideoMode(params[0].getInt(),params[1].getInt(),
                              24,0);//SDL_NOFRAME);
    if(!screen)
        throw "cannot open screen";
    SDL_Flip(screen);
    SDL_Flip(screen);
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

%word col 3 set colour (r g b --)
{
    colr = (uint8_t)params[0].getInt();
    colg = (uint8_t)params[1].getInt();
    colb = (uint8_t)params[2].getInt();
}

%word fillrect 4 (x y w h --) draw a filled rectangle in current colour
{
    chkscr();
    
    int x = params[0].getInt();
    int y = params[1].getInt();
    int w = params[2].getInt();
    int h = params[3].getInt();
    
    SDL_LockSurface(screen);
    int bpp = screen->format->BytesPerPixel;
    for(int j=0;j<h;j++,y++){
        uint8_t *p = (uint8_t*)screen->pixels+y*screen->pitch+x*bpp;
        for(int i=0;i<w;i++){
            p[0] = colb;
            p[1] = colg;
            p[2] = colr;
            p+=bpp;
        }
    }
    SDL_UnlockSurface(screen);
}
        

%init
{
    printf("Initialising SDL plugin, %s %s\n",__DATE__,__TIME__);
}
