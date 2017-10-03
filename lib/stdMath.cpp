#include <math.h>
#include <time.h>
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

%word round (x -- x to nearest int, as a double)
{
    FN(round);
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
    if(v->t == Types::tInteger)
        a->pushInt(ABS(v->toInt()));
    else if(v->t == Types::tDouble)
        a->pushDouble(ABS(v->toDouble()));
    else if(v->t == Types::tLong)
        a->pushLong(ABS(v->toLong()));
    else if(v->t == Types::tFloat)
        a->pushFloat(ABS(v->toFloat()));
    else 
        throw RUNT(EX_TYPE,"").set("Bad type for abs(): %s",v->t->name);
}

%word fmod (x y -- fmod(x,y))
{
    double y = a->popDouble();
    double x = a->popDouble();
    a->pushDouble(fmod(x,y));
}


%word srand (time/none --) set the random number generator seed. 
Sets the random number generator seed. If none, use the timestamp.
{
    Value *v = a->popval();
    long t;
    if(v->isNone())
        time(&t);
    else
        t = v->toInt();
    srand(t);
    srand48(t);
}


%word rand (-- i) stack an integer random number
Stacks an integer random number from 0 to the maximum integer. Typically
used in "rand N %" to give a number from 0 to N-1.
{
    a->pushInt(rand());
}

%word frand (-- random double [0.1])
Uses drand48 to generate a random number from 0 to 1.
{
    a->pushDouble(drand48());
}




// Knuth, TAOCP vol2 p.122
inline double rand_gauss (double mean,double sigma) {
    double v1,v2,s;
    
    do {
        v1 = drand48()*2-1;
        v2 = drand48()*2-1;
        s = v1*v1 + v2*v2;
    } while ( s >= 1.0 );
    
    if (s == 0.0)
        s=0.0;
    else
        s=(v1*sqrt(-2.0 * log(s) / s));
    return (s*sigma)+mean;
}

// http://azzalini.stat.unipd.it/SN/
inline double skewnormal(double location,double scale, double shape){
    double p = rand_gauss(0,1);
    double q = rand_gauss(0,1);
    if(q>shape*p)
        p*=-1;
    return location+scale*p;
}

%wordargs normrand nn (mean sigma -- normally distributed random number)
Generate a normally-distributed random number, as a double.
{
    a->pushDouble(rand_gauss(p0,p1));
}

%wordargs skewnormrand nnn (location scale shape -- n) Skew-normal random number
Generate a skew-normal distributed random number as a double.
{
    a->pushDouble(skewnormal(p0,p1,p2));
}

