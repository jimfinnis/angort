#include <math.h>
#include "angort.h"

using namespace angort;
/*
 * Mappings for (some) standard maths library functions
 */


// macro for helping generate unary float functions
#define FN(f) a->pushFloat(f(a->popFloat()))

%name stdmath

%word cos (x -- cos x)
{
    FN(cosf);
}
%word sin (x -- sin x)
{
    FN(sinf);
}
%word tan (x -- tan x)
{
    FN(tanf);
}
%word ln (x -- ln x)
{
    FN(logf);
}
%word log (x -- ln x)
{
    FN(log10f);
}
%word log2 (x -- log2 x)
{
    FN(log2f);
}
%word sqrt (x -- sqrt x)
{
    FN(sqrtf);
}

%word exp (x -- exp x)
{
    FN(exp);
}

%word pow (x y -- x^y)
{
    float y = a->popFloat();
    float x = a->popFloat();
    a->pushFloat(powf(x,y));
}

#define ABS(x) ((x)<0 ? -(x) : (x))

%word abs (x -- abs(x))
{
    Value *v = a->popval();
    if(v->t == Types::tInteger){
        a->pushInt(ABS(v->toInt()));
    } else 
        a->pushFloat(ABS(v->toFloat()));
}

%word fmod (x y -- fmod(x,y))
{
    float y = a->popFloat();
    float x = a->popFloat();
    a->pushFloat(fmodf(x,y));
}
