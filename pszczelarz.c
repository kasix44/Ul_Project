#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include "hive.h"

/* Zmienne globalne (tylko dla pszczelarza) */
static volatile sig_atomic_t g_stop     = 0;
static volatile sig_atomic_t g_increase = 0;
static volatile sig_atomic_t g_decrease = 0;

/* Handlery sygnałów */
static void sigterm_handler(int signo) {
    (void) signo;
    g_stop = 1;
}

static void sigusr1_handler(int signo) {
    (void) signo;
    g_increase = 1;
}

static void sigusr2_handler(int signo) {
    (void) signo;
    g_decrease = 1;
}

void beekeeper_proc(hive_t *hive, int semid)
{
    /* Ustawiamy obsługę sygnałów */
    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    sa.sa_handler = sigterm_handler;
    sigaction(SIGTERM, &sa, NULL);

    sa.sa_handler = sigusr1_handler;
    sigaction(SIGUSR1, &sa, NULL);

    sa.sa_handler = sigusr2_handler;
    sigaction(SIGUSR2, &sa, NULL);

    printf("[PSZCZELARZ] Start (PID=%d)\n", getpid());

    while (!g_stop) {
        pause(); /* czekamy na sygnał */
        if (g_stop) {
            break;
        }

        if (g_increase) {
            g_increase = 0;
            sem_down(semid, SEM_MUTEX);
            int old_cap = hive->max_capacity;
            hive->max_capacity = 2 * hive->N;
            if (hive->max_capacity > MAX_BEES) {
                hive->max_capacity = MAX_BEES;
            }
            /* ustawiamy nową wartość semafora SEM_CAP */
            union semun {
                int val;
                struct semid_ds *buf;
                unsigned short *array;
            } arg;
            arg.val = hive->max_capacity;
            if (semctl(semid, SEM_CAP, SETVAL, arg) < 0) {
                perror("[PSZCZELARZ] semctl SETVAL (SIGUSR1)");
            }
            printf("[PSZCZELARZ] SIGUSR1 -> pojemność z %d na %d\n",
                   old_cap, hive->max_capacity);
            sem_up(semid, SEM_MUTEX);
        }

        if (g_decrease) {
            g_decrease = 0;
            sem_down(semid, SEM_MUTEX);
            int old_cap = hive->max_capacity;
            int new_cap = old_cap / 2;
            if (new_cap < 1) {
                new_cap = 1;
            }
            hive->max_capacity = new_cap;

            union semun {
                int val;
                struct semid_ds *buf;
                unsigned short *array;
            } arg;
            arg.val = hive->max_capacity;
            if (semctl(semid, SEM_CAP, SETVAL, arg) < 0) {
                perror("[PSZCZELARZ] semctl SETVAL (SIGUSR2)");
            }
            printf("[PSZCZELARZ] SIGUSR2 -> pojemność z %d na %d\n",
                   old_cap, hive->max_capacity);
            sem_up(semid, SEM_MUTEX);
        }
    }

    printf("[PSZCZELARZ] Koniec.\n");
}
