#ifndef BEE_H
#define BEE_H

#include "common.h"

typedef struct {
    int id;         // ID pszczoły
    int visits;     // ile razy pszczoła odwiedziła ul
    int maxVisits;  // po ilu wizytach umiera
    int T_inHive;   // czas przebywania pszczoły w ulu
    HiveData* hive;
} BeeArgs;

void* beeWorker(void* arg);

#endif
