#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "hive.h"

/* Funkcje lokalne do wejścia/wyjścia przez wąskie wejścia */

void enter_hive(hive_t *hive, int semid)
{
    /* Najpierw musimy mieć miejsce w ulu */
    sem_down(semid, SEM_CAP);

    /* Losowy wybór wejścia (0 lub 1) */
    int ent = rand() % 2;

    while (1) {
        sem_down(semid, SEM_MUTEX);

        if (hive->direction[ent] == 0) {
            /* wejście wolne -> ustawiamy inbound */
            hive->direction[ent] = 1;
            hive->count[ent] = 1;
            sem_up(semid, SEM_MUTEX);
            break;
        } else if (hive->direction[ent] == 1) {
            /* też inbound -> dołączamy się */
            hive->count[ent]++;
            sem_up(semid, SEM_MUTEX);
            break;
        } else {
            /* jest outbound */
            hive->wait_inbound[ent]++;
            sem_up(semid, SEM_MUTEX);

            /* czekamy na semafor inbound */
            if (ent == 0) {
                sem_down(semid, SEM_ENT0_IN);
            } else {
                sem_down(semid, SEM_ENT1_IN);
            }
            /* i wracamy do pętli */
        }
    }

    /* Teraz zwiększamy current_inside */
    sem_down(semid, SEM_MUTEX);
    hive->current_inside++;
    /* Pszczoła już przeszła przez wejście, zmniejszamy count */
    hive->count[ent]--;

    if (hive->count[ent] == 0) {
        /* zwalniamy wejście, wybudzamy czekających */
        hive->direction[ent] = 0;
        /* budzimy inbound czekających */
        while (hive->wait_inbound[ent] > 0) {
            hive->wait_inbound[ent]--;
            if (ent == 0) sem_up(semid, SEM_ENT0_IN);
            else          sem_up(semid, SEM_ENT1_IN);
        }
        /* budzimy outbound czekających */
        while (hive->wait_outbound[ent] > 0) {
            hive->wait_outbound[ent]--;
            if (ent == 0) sem_up(semid, SEM_ENT0_OUT);
            else          sem_up(semid, SEM_ENT1_OUT);
        }
    }
    sem_up(semid, SEM_MUTEX);
}

void exit_hive(hive_t *hive, int semid)
{
    /* najpierw pszczoła wychodzi z ula -> current_inside--, sem_up(SEM_CAP) */
    sem_down(semid, SEM_MUTEX);
    hive->current_inside--;
    sem_up(semid, SEM_MUTEX);

    sem_up(semid, SEM_CAP);

    /* losowo wybieramy wejście do wyjścia (outbound) */
    int ent = rand() % 2;

    while (1) {
        sem_down(semid, SEM_MUTEX);

        if (hive->direction[ent] == 0) {
            hive->direction[ent] = 2; /* outbound */
            hive->count[ent] = 1;
            sem_up(semid, SEM_MUTEX);
            break;
        } else if (hive->direction[ent] == 2) {
            hive->count[ent]++;
            sem_up(semid, SEM_MUTEX);
            break;
        } else {
            /* jest inbound */
            hive->wait_outbound[ent]++;
            sem_up(semid, SEM_MUTEX);

            if (ent == 0) {
                sem_down(semid, SEM_ENT0_OUT);
            } else {
                sem_down(semid, SEM_ENT1_OUT);
            }
        }
    }

    /* Teraz pszczoła przeszła, zmniejszamy count */
    sem_down(semid, SEM_MUTEX);
    hive->count[ent]--;

    if (hive->count[ent] == 0) {
        /* zwalniamy wejście */
        hive->direction[ent] = 0;
        while (hive->wait_inbound[ent] > 0) {
            hive->wait_inbound[ent]--;
            if (ent == 0) sem_up(semid, SEM_ENT0_IN);
            else          sem_up(semid, SEM_ENT1_IN);
        }
        while (hive->wait_outbound[ent] > 0) {
            hive->wait_outbound[ent]--;
            if (ent == 0) sem_up(semid, SEM_ENT0_OUT);
            else          sem_up(semid, SEM_ENT1_OUT);
        }
    }
    sem_up(semid, SEM_MUTEX);
}

void bee_proc(hive_t *hive, int semid, int bee_id)
{
    srand(getpid() ^ time(NULL));
    int Ti = 1 + (rand() % 5);          /* 1..5 s w ulu */
    int Xi = 1 + (rand() % MAX_VISITS); /* liczba odwiedzin 1..MAX_VISITS */

    printf("[PSZCZOŁA %d PID=%d] Start; Ti=%d, Xi=%d\n", bee_id, getpid(), Ti, Xi);

    int visits = 0;
    while (1) {
        /* wejście do ula */
        enter_hive(hive, semid);
        visits++;

        printf("[PSZCZOŁA %d] -> w ulu (visit %d/%d), inside=%d\n",
               bee_id, visits, Xi, hive->current_inside);

        sleep(Ti); /* pszczoła spędza czas w ulu */

        /* wyjście z ula */
        exit_hive(hive, semid);
        printf("[PSZCZOŁA %d] <- opuściła ul, inside=%d\n",
               bee_id, hive->current_inside);

        if (visits >= Xi) {
            /* umiera */
            sem_down(semid, SEM_MUTEX);
            hive->living_bees--;
            printf("[PSZCZOŁA %d] Umieram... (living=%d)\n", bee_id, hive->living_bees);
            sem_up(semid, SEM_MUTEX);
            break;
        }

        /* poza ulem krótka przerwa */
        sleep(1 + (rand() % 3));
    }
}
