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

int fxpt(int a) {
	return a * F;
}

int floor(int a) {
	return a / F;
}

int round(int a) {
	if (a>=0)
		return (a + F/2) / F;
	else
		return (a - F/2) / F;
}

int addint(int a, int n) {
	return a + (n * F);
}

int addfxpt(int a, int b) {
	return a + b;
}

int subint(int a, int b) {
	return a - (b * F);
}

int subfxpt(int a, int b) {
	return a - b;
}

int mulint(int a, int b) {
	return a * b;
}

int mulfxpt(int a, int b) {
	return ((int64_t) a) * b / F;
}

int divint(int a, int b) {
	return a / b;
}

int divfxpt(int a, int b) {
	return ((int64_t) a) * F / b;
}