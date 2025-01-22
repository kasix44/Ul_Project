#ifndef HIVE_H
#define HIVE_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <errno.h>

/* Stałe */
#define MAX_BEES   200
#define MAX_VISITS 10
#define SEM_COUNT  6

/* Indeksy semaforów */
enum {
    SEM_MUTEX = 0,
    SEM_CAP,
    SEM_ENT0_IN,
    SEM_ENT0_OUT,
    SEM_ENT1_IN,
    SEM_ENT1_OUT
};

/* Struktura „ula” w pamięci dzielonej */
typedef struct {
    int N;
    int P;
    int current_inside;
    int max_capacity;
    int total_bees;
    int living_bees;
    int queen_interval;

    /* Stan dwóch wejść (0 i 1): 0=idle, 1=inbound, 2=outbound */
    int direction[2];
    int count[2];
    int wait_inbound[2];
    int wait_outbound[2];
} hive_t;

/* Funkcje z hive.c */
int  init_system(int N, int P, int T_k, int *shmid_out, int *semid_out, hive_t **ptr_out);
void cleanup_system(int shmid, int semid, hive_t *ptr);

/* Funkcje z sync.c */
void sem_down(int semid, int sem_num);
void sem_up(int semid, int sem_num);

#endif
