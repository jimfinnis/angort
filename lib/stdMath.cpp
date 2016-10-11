#include <math.h>
#include "angort.h"

using namespace angort;
/*
 * Mappings for (some) standard maths library functions
 */


// macro for helping generate unary float functions
#define FN(f) a->pushDouble(f(a->popDouble()))

%name stdmath

%wordargs atan2 dd (y x -- atan2(y,x))
{
    a->pushDouble(atan2(p0,p1));
}
    

%word cos (x -- cos x)
{
    FN(cos);
}
%word sin (x -- sin x)
{
    FN(sin);
}
%word tan (x -- tan x)
{
    FN(tan);
}
%word ln (x -- ln x)
{
    FN(log);
}
%word log (x -- ln x)
{
    FN(log10);
}
%word log2 (x -- log2 x)
{
    FN(log2);
}
%word sqrt (x -- sqrt x)
{
    FN(sqrt);
}

%word exp (x -- exp x)
{
    FN(exp);
}

%word pow (x y -- x^y)
{
    double y = a->popDouble();
    double x = a->popDouble();
    a->pushDouble(pow(x,y));
}

#define ABS(x) ((x)<0 ? -(x) : (x))

%word abs (x -- abs(x))
{
    Value *v = a->popval();
    if(v->t == Types::tInteger){
        a->pushInt(ABS(v->toInt()));
    } else 
        a->pushDouble(ABS(v->toDouble()));
}

%word fmod (x y -- fmod(x,y))
{
    double y = a->popDouble();
    double x = a->popDouble();
    a->pushDouble(fmod(x,y));
}
