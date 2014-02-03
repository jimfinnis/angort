#include <math.h>
#include "angort.h"


#define FN(f) a->pushFloat(f(a->popFloat()))
%name stdmath


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
