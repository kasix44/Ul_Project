#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pthread.h>

#include "hive.h"

/* Flaga globalna, że mamy się zakończyć */
static volatile sig_atomic_t g_killme = 0;

/* Wątek czyściciela zombie */
static void* reaper_thread(void *arg)
{
    (void)arg;
    while (!g_killme) {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid > 0) {
            // Zebrano zakończone dziecko
        } else if (pid == 0) {
            // Nic do zebrania
            usleep(200000); // 0.2 s
        } else {
            // Błąd lub brak dzieci
            usleep(200000);
        }
    }
    return NULL;
}

/* Handler SIGTERM -> kończymy pętlę */
static void sigterm_handler(int signo) {
    (void)signo;
    printf("[KROLOWA] otrzymała SIGTERM, kończę...\n");
    g_killme = 1;
}

/* Właściwa logika królowej */
void queen_proc(hive_t *hive, int semid)
{
    /* Obsługa sygnału SIGTERM */
    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = sigterm_handler;
    sigaction(SIGTERM, &sa, NULL);

    /* Uruchamiamy wątek czyściciela zombie */
    pthread_t tid;
    pthread_create(&tid, NULL, reaper_thread, NULL);

    printf("[KROLOWA] Start (PID=%d), co %d sekund składam jaja.\n",
           getpid(), hive->queen_interval);

    while (!g_killme) {
        sleep(hive->queen_interval);

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
                /* dziecko = nowa pszczoła */
                sem_down(semid, SEM_MUTEX);
                int new_id = hive->total_bees + 1;
                hive->total_bees++;
                hive->living_bees++;
                printf("[KROLOWA] Nowa pszczoła #%d (PID=%d)\n", new_id, getpid());
                sem_up(semid, SEM_MUTEX);

                /* Uruchamiamy TEGO SAMEGO binarka z argv[0] = "pszczola" */
                char shm_str[32], sem_str[32], bee_str[32];
                snprintf(shm_str, sizeof(shm_str), "%d",
                         shmget(ftok("/tmp", 81), 0, 0)); /* aktualny shmid */
                snprintf(sem_str, sizeof(sem_str), "%d", semid);
                snprintf(bee_str, sizeof(bee_str), "%d", new_id);

                execl("./ul", "pszczola", shm_str, sem_str, bee_str, (char*)NULL);
                perror("[KROLOWA] execl pszczola");
                _exit(1);
            }
            /* rodzic = królowa -> wracamy do pętli */
        }
    }

    /* Kończymy wątek reaper */
    g_killme = 1;
    pthread_join(tid, NULL);

    printf("[KROLOWA] Koniec.\n");
}
