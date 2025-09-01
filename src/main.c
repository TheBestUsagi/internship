#include <stdio.h>


 int main(void){

#if defined(__aarch64__)
    puts("Hello, arch=aarch64");
#elif defined(__arm__)
    puts("Hello, arch=arm");
#elif defined(__x86_64__)
    puts("Hello, arch=x86_64");
#else
    puts("Hello, arch=unknown");
#endif
    return 0;
 }
