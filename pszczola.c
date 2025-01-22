#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "hive.h"

void enter_hive(hive_t *hive, int semid)
{
    sem_down(semid, SEM_CAP);
    int ent = rand() % 2;
    while (1) {
        sem_down(semid, SEM_MUTEX);
        if (hive->direction[ent] == 0) {
            hive->direction[ent] = 1; // inbound
            hive->count[ent] = 1;
            sem_up(semid, SEM_MUTEX);
            break;
        } else if (hive->direction[ent] == 1) {
            hive->count[ent]++;
            sem_up(semid, SEM_MUTEX);
            break;
        } else {
            // outbound -> czekamy
            hive->wait_inbound[ent]++;
            sem_up(semid, SEM_MUTEX);
            if (ent == 0) sem_down(semid, SEM_ENT0_IN);
                     else sem_down(semid, SEM_ENT1_IN);
        }
    }

    sem_down(semid, SEM_MUTEX);
    hive->current_inside++;
    hive->count[ent]--;
    if (hive->count[ent] == 0) {
        hive->direction[ent] = 0;
        while (hive->wait_inbound[ent] > 0) {
            hive->wait_inbound[ent]--;
            if (ent == 0) sem_up(semid, SEM_ENT0_IN);
                    else  sem_up(semid, SEM_ENT1_IN);
        }
        while (hive->wait_outbound[ent] > 0) {
            hive->wait_outbound[ent]--;
            if (ent == 0) sem_up(semid, SEM_ENT0_OUT);
                    else  sem_up(semid, SEM_ENT1_OUT);
        }
    }
    sem_up(semid, SEM_MUTEX);
}

void exit_hive(hive_t *hive, int semid)
{
    sem_down(semid, SEM_MUTEX);
    hive->current_inside--;
    sem_up(semid, SEM_MUTEX);

    sem_up(semid, SEM_CAP);

    int ent = rand() % 2;
    while (1) {
        sem_down(semid, SEM_MUTEX);
        if (hive->direction[ent] == 0) {
            hive->direction[ent] = 2; // outbound
            hive->count[ent] = 1;
            sem_up(semid, SEM_MUTEX);
            break;
        } else if (hive->direction[ent] == 2) {
            hive->count[ent]++;
            sem_up(semid, SEM_MUTEX);
            break;
        } else {
            // inbound -> czekamy
            hive->wait_outbound[ent]++;
            sem_up(semid, SEM_MUTEX);
            if (ent == 0) sem_down(semid, SEM_ENT0_OUT);
                     else sem_down(semid, SEM_ENT1_OUT);
        }
    }

    sem_down(semid, SEM_MUTEX);
    hive->count[ent]--;
    if (hive->count[ent] == 0) {
        hive->direction[ent] = 0;
        while (hive->wait_inbound[ent] > 0) {
            hive->wait_inbound[ent]--;
            if (ent == 0) sem_up(semid, SEM_ENT0_IN);
                    else  sem_up(semid, SEM_ENT1_IN);
        }
        while (hive->wait_outbound[ent] > 0) {
            hive->wait_outbound[ent]--;
            if (ent == 0) sem_up(semid, SEM_ENT0_OUT);
                    else  sem_up(semid, SEM_ENT1_OUT);
        }
    }
    sem_up(semid, SEM_MUTEX);
}

void bee_proc(hive_t *hive, int semid, int bee_id)
{
    srand(getpid() ^ time(NULL));
    int Ti = 1 + (rand() % 5);          // 1..5 s w ulu
    int Xi = 1 + (rand() % MAX_VISITS); // 1..MAX_VISITS

    printf("[PSZCZOŁA %d PID=%d] Start; Ti=%d, Xi=%d\n",
           bee_id, getpid(), Ti, Xi);

    int visits = 0;
    while (1) {
        enter_hive(hive, semid);
        visits++;

        int inside  = hive->current_inside;
        int outside = hive->living_bees - inside;
        printf("[PSZCZOŁA %d] -> w ulu (visit %d/%d), inside=%d, outside=%d\n",
               bee_id, visits, Xi, inside, outside);

        sleep(Ti);

        exit_hive(hive, semid);
        inside  = hive->current_inside;
        outside = hive->living_bees - inside;
        printf("[PSZCZOŁA %d] <- opuściła ul, inside=%d, outside=%d\n",
               bee_id, inside, outside);

        if (visits >= Xi) {
            sem_down(semid, SEM_MUTEX);
            hive->living_bees--;
            printf("[PSZCZOŁA %d] Umieram... (living=%d)\n",
                   bee_id, hive->living_bees);
            sem_up(semid, SEM_MUTEX);
            break;
        }

        sleep(1 + (rand() % 3));
    }
}
