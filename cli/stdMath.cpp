#include <math.h>
#include "angort.h"


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
