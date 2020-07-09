#include "resampler-table.h"



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>


int zita_resampler_major_version(void)
{
    return ZITA_RESAMPLER_MAJOR_VERSION;
}


int zita_resampler_minor_version(void)
{
    return ZITA_RESAMPLER_MINOR_VERSION;
}


static double sinc(double x)
{
    x = fabs(x);
    if (x < 1e-6) return 1.0;
    x *= M_PI;
    return sin(x) / x;
}


static double wind(double x)
{
    x = fabs(x);
    if (x >= 1.0) return 0.0f;
    x *= M_PI;
    return 0.384 + 0.500 * cos(x) + 0.116 * cos(2 * x);
}



Resampler_table* Resampler_table::_list = 0;
Resampler_mutex   Resampler_table::_mutex;


Resampler_table::Resampler_table(double fr, unsigned int hl, unsigned int np) :
    _next(0),
    _refc(0),
    _fr(fr),
    _hl(hl),
    _np(np)
{
    unsigned int  i, j;
    double        t;
    float* p;

    _ctab = new float[hl * (np + 1)];
    p = _ctab;
    for (j = 0; j <= np; j++)
    {
        t = (double)j / (double)np;
        for (i = 0; i < hl; i++)
        {
            p[hl - i - 1] = (float)(fr * sinc(t * fr) * wind(t / hl));
            t += 1;
        }
        p += hl;
    }
}


Resampler_table::~Resampler_table(void)
{
    delete[] _ctab;
}


Resampler_table* Resampler_table::create(double fr, unsigned int hl, unsigned int np)
{
    Resampler_table* P;

    _mutex.lock();
    P = _list;
    while (P)
    {
        if ((fr >= P->_fr * 0.999) && (fr <= P->_fr * 1.001) && (hl == P->_hl) && (np == P->_np))
        {
            P->_refc++;
            _mutex.unlock();
            return P;
        }
        P = P->_next;
    }
    P = new Resampler_table(fr, hl, np);
    P->_refc = 1;
    P->_next = _list;
    _list = P;
    _mutex.unlock();
    return P;
}


void Resampler_table::destroy(Resampler_table* T)
{
    Resampler_table* P, * Q;

    _mutex.lock();
    if (T)
    {
        T->_refc--;
        if (T->_refc == 0)
        {
            P = _list;
            Q = 0;
            while (P)
            {
                if (P == T)
                {
                    if (Q) Q->_next = T->_next;
                    else      _list = T->_next;
                    break;
                }
                Q = P;
                P = P->_next;
            }
            delete T;
        }
    }
    _mutex.unlock();
}


void Resampler_table::print_list(void)
{
    Resampler_table* P;

    printf("Resampler table\n----\n");
    for (P = _list; P; P = P->_next)
    {
        printf("refc = %3d   fr = %10.6lf  hl = %4d  np = %4d\n", P->_refc, P->_fr, P->_hl, P->_np);
    }
    printf("----\n\n");
}

