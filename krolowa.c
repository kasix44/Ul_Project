/* krolowa.c -> kompiluje się do 'krolowa' */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pthread.h>

#include "hive.h"

/* Flaga globalna – zatrzymanie królowej */
static volatile sig_atomic_t g_stop = 0;

/* Wątek reaper – zbiera zakończone dzieci królowej (pszczoły) */
static void* reaper_thread(void *arg)
{
    (void)arg;
    while (!g_stop) {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid > 0) {
            // zebrano zakończonego potomka
        } else if (pid == 0) {
            // brak do zbierania
            usleep(200000);
        } else {
            // brak dzieci lub błąd
            usleep(200000);
        }
    }
    return NULL;
}

/* Handler SIGTERM */
static void sigterm_handler(int signo) {
    (void)signo;
    printf("[KROLOWA] otrzymała SIGTERM, kończę...\n");
    g_stop = 1;
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Użycie: %s <shmid> <semid>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int shmid = atoi(argv[1]);
    int semid = atoi(argv[2]);

    hive_t *hive = (hive_t*) shmat(shmid, NULL, 0);
    if (hive == (void*)-1) {
        perror("shmat krolowa");
        exit(EXIT_FAILURE);
    }

    /* Obsługa SIGTERM */
    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = sigterm_handler;
    sigaction(SIGTERM, &sa, NULL);

    /* Uruchamiamy wątek reaper */
    pthread_t tid;
    pthread_create(&tid, NULL, reaper_thread, NULL);

    printf("[KROLOWA] Start (PID=%d), co %d sekund składam jaja.\n",
           getpid(), hive->queen_interval);

    while (!g_stop) {
        sleep(hive->queen_interval);

        /* Sprawdzamy, czy w ulu jest miejsce */
        sem_down(semid, SEM_MUTEX);
        int canLay = (hive->living_bees < hive->max_capacity);
        sem_up(semid, SEM_MUTEX);

        if (canLay) {
            pid_t pid = fork();
            if (pid < 0) {
                perror("[KROLOWA] fork error");
                continue;
            }
            if (pid == 0) {
                /* dziecko – nowa pszczoła */
                sem_down(semid, SEM_MUTEX);
                int new_id = hive->total_bees + 1;
                hive->total_bees++;
                hive->living_bees++;
                printf("[KROLOWA] Nowa pszczoła #%d (PID=%d)\n",
                       new_id, getpid());
                sem_up(semid, SEM_MUTEX);

                char shm_str[32], sem_str[32], bee_str[32];
                snprintf(shm_str, sizeof(shm_str), "%d", shmid);
                snprintf(sem_str, sizeof(sem_str), "%d", semid);
                snprintf(bee_str, sizeof(bee_str), "%d", new_id);

                execl("./pszczola", "pszczola", shm_str, sem_str, bee_str, (char*)NULL);
                perror("[KROLOWA] execl pszczola");
                _exit(1);
            }
            /* rodzic -> królowa */
        }
    }

    /* Kończymy wątek reaper */
    g_stop = 1;
    pthread_join(tid, NULL);

    printf("[KROLOWA] Koniec.\n");

    shmdt(hive);
    return 0;
}
