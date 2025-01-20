#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "hive.h"

void queen_proc(hive_t *hive, int semid)
{
    printf("[KRÓLOWA] Start (PID=%d), co %d sekund składam jaja.\n",
           getpid(), hive->queen_interval);

    /* opcjonalnie ignorujemy "zombie" */
    signal(SIGCHLD, SIG_IGN);

    while (1) {
        sleep(hive->queen_interval);

        /* sprawdzamy, czy jest miejsce */
        sem_down(semid, SEM_MUTEX);
        int canLay = 0;
        if (hive->living_bees < hive->max_capacity) {
            canLay = 1;
        }
        sem_up(semid, SEM_MUTEX);

        if (canLay) {
            pid_t pid = fork();
            if (pid < 0) {
                perror("[KRÓLOWA] fork error");
                continue;
            }
            if (pid == 0) {
                /* dziecko = nowa pszczoła */
                sem_down(semid, SEM_MUTEX);
                int new_id = hive->total_bees + 1;
                hive->total_bees++;
                hive->living_bees++;
                printf("[KRÓLOWA] Nowa pszczoła #%d (PID=%d), living=%d\n",
                       new_id, getpid(), hive->living_bees);
                sem_up(semid, SEM_MUTEX);

                bee_proc(hive, semid, new_id);
                _exit(0);
            }
        } else {
            // printf("[KRÓLOWA] Brak miejsca na nowe jajo.\n");
        }
    }
}
