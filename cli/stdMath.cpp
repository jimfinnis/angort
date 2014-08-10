#include <math.h>
#include "angort.h"

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

%word fmod (x y -- fmod(x,y))
{
    float y = a->popFloat();
    float x = a->popFloat();
    a->pushFloat(fmodf(x,y));
}
