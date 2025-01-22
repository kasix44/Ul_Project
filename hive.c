#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>

#include "hive.h"

union semun {
    int val;                 // wartośc dla SETVAL
    struct semid_ds *buf;    // bufor dla IPC_STAT, IPC_SET
    unsigned short *array;   // tablica dla GETALL, SETALL
    struct seminfo *__buf;   // bufor dla IPC_INFO 
};

int init_system(int N, int P, int T_k, int *shmid_out, int *semid_out, hive_t **ptr_out)
{
    key_t key_shm = ftok("/tmp", 81);
    if (key_shm == -1) {
        perror("ftok for shm");
        return -1;
    }

    int shmid = shmget(key_shm, sizeof(hive_t), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        return -1;
    }

    hive_t *hive = (hive_t*)shmat(shmid, NULL, 0);
    if (hive == (void*)-1) {
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);
        return -1;
    }

    /* Inicjalizacja wartości w pamięci dzielonej */
    hive->N              = N;
    hive->P              = P;
    hive->current_inside = 0;
    hive->max_capacity   = P;
    hive->total_bees     = N;
    hive->living_bees    = N;
    hive->queen_interval = T_k;

    for (int i = 0; i < 2; i++) {
        hive->direction[i]     = 0;
        hive->count[i]         = 0;
        hive->wait_inbound[i]  = 0;
        hive->wait_outbound[i] = 0;
    }

    /* Tworzenie semaforów */
    key_t key_sem = ftok("/tmp", 82);
    if (key_sem == -1) {
        perror("ftok for sem");
        shmdt(hive);
        shmctl(shmid, IPC_RMID, NULL);
        return -1;
    }

    int semid = semget(key_sem, SEM_COUNT, IPC_CREAT | 0666);
    if (semid < 0) {
        perror("semget");
        shmdt(hive);
        shmctl(shmid, IPC_RMID, NULL);
        return -1;
    }

    union semun arg;

    /* SEM_MUTEX = 1 */
    arg.val = 1;
    if (semctl(semid, SEM_MUTEX, SETVAL, arg) < 0) {
        perror("semctl SEM_MUTEX");
        goto fail;
    }

    /* SEM_CAP = P */
    arg.val = P;
    if (semctl(semid, SEM_CAP, SETVAL, arg) < 0) {
        perror("semctl SEM_CAP");
        goto fail;
    }

    /* SEM_ENT0_IN = SEM_ENT0_OUT = SEM_ENT1_IN = SEM_ENT1_OUT = 0 */
    arg.val = 0;
    if (semctl(semid, SEM_ENT0_IN,  SETVAL, arg) < 0) goto fail;
    if (semctl(semid, SEM_ENT0_OUT, SETVAL, arg) < 0) goto fail;
    if (semctl(semid, SEM_ENT1_IN,  SETVAL, arg) < 0) goto fail;
    if (semctl(semid, SEM_ENT1_OUT, SETVAL, arg) < 0) goto fail;

    *shmid_out = shmid;
    *semid_out = semid;
    *ptr_out   = hive;
    return 0;

fail:
    shmdt(hive);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
    return -1;
}

void cleanup_system(int shmid, int semid, hive_t *ptr)
{
    if (ptr != (void*)-1) {
        shmdt(ptr);
    }
    if (shmid >= 0) {
        shmctl(shmid, IPC_RMID, NULL);
    }
    if (semid >= 0) {
        semctl(semid, 0, IPC_RMID);
    }
}
