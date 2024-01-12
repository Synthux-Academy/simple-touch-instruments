#pragma once

/** https://cplusplus.com/reference/cstdlib/rand/
 * 
 * v1 = rand() % 100;         // v1 in the range 0 to 99
 * v2 = rand() % 100 + 1;     // v2 in the range 1 to 100
 * v3 = rand() % 30 + 1985;   // v3 in the range 1985-2014 
 */ 
int randomrange(int start, int stop){
    int v = rand() % (stop-start) + start;
    return v;
}