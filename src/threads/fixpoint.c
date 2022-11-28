#include "fixpoint.h"

int fxpt(int a) { return a*F;}
int floor(int a) { return a/F;}
int round(int a) {return a>=0? (a+F/2)/F : (a-F/2)/F;}
int addint(int a, int n) {return a + (n * F);}
int addfxpt(int a, int b) {return a + b;}
int subint(int a, int n) {return a - (n * F);}
int subfxpt(int a, int b) {return a - b;}
int mulint(int a, int b) {return a * b;}
int mulfxpt(int a, int n) {return ((int64_t) a) * n / F;}
int divint(int a, int b) {return a / b;}
int divfxpt(int a, int n) {return ((int64_t) a) * F / n;}