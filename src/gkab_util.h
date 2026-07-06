#ifndef GKAB_UTIL
#define GKAB_UTIL

#include <stdlib.h>

int gkab_rand_int(int lower, int upper) {
    assert(upper >= lower);
    return rand() % (upper - lower + 1) + lower;
}

#endif //GKAB_UTIL
