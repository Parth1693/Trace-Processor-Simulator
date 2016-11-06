#include <stdio.h>

union DOUBLE_WORD {
   unsigned long long dw;
   long w[2];
   float f[2];
   double d;
};

main() {
DOUBLE_WORD x;

//printf("sizeof(unsigned long long) = %d\n", sizeof(unsigned long long));

x.dw = 16;
//x.d = (double)-546.9999555;

printf("x.dw   = %llx\n", x.dw);

printf("x.w[1] = %x\n", x.w[1]);
printf("x.w[0] = %x\n", x.w[0]);

printf("x.f[1] = %f\n", x.f[1]);
printf("x.f[0] = %f\n", x.f[0]);

printf("x.d    = %f\n", x.d);
}
