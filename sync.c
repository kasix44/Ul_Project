#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <errno.h>
#include "hive.h"

void sem_down(int semid, int sem_num)
{
    struct sembuf op;
    op.sem_num = sem_num;
    op.sem_op  = -1;
    op.sem_flg = 0;

    while (semop(semid, &op, 1) < 0) {
        if (errno != EINTR) {
            perror("semop down");
            break;
        }
    }
}

void sem_up(int semid, int sem_num)
{
    struct sembuf op;
    op.sem_num = sem_num;
    op.sem_op  = 1;
    op.sem_flg = 0;

    while (semop(semid, &op, 1) < 0) {
        if (errno != EINTR) {
            perror("semop up");
            break;
        }
    }
}
