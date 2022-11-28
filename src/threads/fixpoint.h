#ifndef THREAD_FIXPOINT_H
#define THREAD_FIXPOINT_H

#include <stdint.h>
#define F 0b100000000000000

int fxpt(int a);
int floor(int a);
int round(int a);
int addint(int a, int b);
int addfxpt(int a, int b);
int subint(int a, int b);
int subfxpt(int a, int b);
int mulint(int a, int b);
int mulfxpt(int a, int b);
int divint(int a, int b);
int divfxpt(int a, int b);

#endif