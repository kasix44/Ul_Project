/* main.c -> kompilujemy do 'ul' */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>

#include "hive.h"

static volatile sig_atomic_t g_running     = 1;
static volatile sig_atomic_t g_reaper_stop = 0;

static pid_t g_pid_beekeeper = -1;
static pid_t g_pid_queen     = -1;
static int   g_shmid = -1, g_semid = -1;
static hive_t *g_hive = NULL;

/* Wątek reaper w PROCESIE GŁÓWNYM – zbiera zakończone dzieci. */
static void* reaper_thread(void *arg)
{
    (void) arg;
    while (!g_reaper_stop) {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid > 0) {
            // zebraliśmy zakończonego potomka
        } else if (pid == 0) {
            // brak do zbierania
            usleep(200000); // 0.2s
        } else {
            // pid < 0 => brak dzieci
            usleep(200000);
        }
    }
    return NULL;
}

/* Handler SIGINT / SIGTERM => kończymy pętlę główną */
static void signal_handler(int signo) {
    (void)signo;
    g_running = 0;
}

/* Funkcja czyszcząca (atexit) */
static void cleanup_all(void)
{
    printf("\n[MAIN] Rozpoczynam zamykanie programu...\n");

    if (g_pid_beekeeper > 0) kill(g_pid_beekeeper, SIGTERM);
    if (g_pid_queen > 0)     kill(g_pid_queen, SIGTERM);

    usleep(300000);

    g_reaper_stop = 1;
    usleep(300000);

    /* Ostateczny wait na pozostałe */
    while (waitpid(-1, NULL, 0) > 0) {
        // nic
    }

    if (g_hive != NULL && g_shmid != -1 && g_semid != -1) {
        cleanup_system(g_shmid, g_semid, g_hive);
    }

    printf("[MAIN] Zakończono.\n");
}

int main(int argc, char *argv[])
{
    if (argc < 4) {
        fprintf(stderr, "Użycie: %s <N> <P> <Tk>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int N  = atoi(argv[1]);
    int P  = atoi(argv[2]);
    int Tk = atoi(argv[3]);

    if (N < 1 || P < 1 || Tk < 1) {
        fprintf(stderr, "Błędne parametry - wszystkie muszą być > 0.\n");
        exit(EXIT_FAILURE);
    }
    if (P >= (N/2)) {
        fprintf(stderr, "Warunek P < N/2 nie jest spełniony!\n");
        exit(EXIT_FAILURE);
    }
    if (N > MAX_BEES) {
        fprintf(stderr, "N zbyt duże (max %d)\n", MAX_BEES);
        exit(EXIT_FAILURE);
    }

    /* Handlery SIGINT, SIGTERM */
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction SIGTERM");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction SIGINT");
        exit(EXIT_FAILURE);
    }

    /* Rejestracja cleanup_all */
    if (atexit(cleanup_all) != 0) {
        fprintf(stderr, "Nie można zarejestrować funkcji atexit\n");
        exit(EXIT_FAILURE);
    }

    /* Inicjalizacja systemu (pamięć + semafory) */
    if (init_system(N, P, Tk, &g_shmid, &g_semid, &g_hive) < 0) {
        fprintf(stderr, "[MAIN] init_system error\n");
        exit(EXIT_FAILURE);
    }

    /* Uruchamiamy wątek reaper */
    pthread_t tid;
    pthread_create(&tid, NULL, reaper_thread, NULL);

    /* Tworzymy proces pszczelarza */
    g_pid_beekeeper = fork();
    if (g_pid_beekeeper < 0) {
        perror("fork pszczelarz");
        exit(EXIT_FAILURE);
    } else if (g_pid_beekeeper == 0) {
        char shm_str[32], sem_str[32];
        snprintf(shm_str, sizeof(shm_str), "%d", g_shmid);
        snprintf(sem_str, sizeof(sem_str), "%d", g_semid);
        execl("./pszczelarz", "pszczelarz", shm_str, sem_str, (char*)NULL);
        perror("execl pszczelarz");
        _exit(1);
    }

    /* Tworzymy proces królowej */
    g_pid_queen = fork();
    if (g_pid_queen < 0) {
        perror("fork krolowa");
        kill(g_pid_beekeeper, SIGTERM);
        exit(EXIT_FAILURE);
    } else if (g_pid_queen == 0) {
        char shm_str[32], sem_str[32];
        snprintf(shm_str, sizeof(shm_str), "%d", g_shmid);
        snprintf(sem_str, sizeof(sem_str), "%d", g_semid);
        execl("./krolowa", "krolowa", shm_str, sem_str, (char*)NULL);
        perror("execl krolowa");
        _exit(1);
    }

    /* Tworzymy N pszczół */
    for (int i = 0; i < N; i++) {
        pid_t pid_b = fork();
        if (pid_b < 0) {
            perror("fork bee");
            exit(EXIT_FAILURE);
        }
        if (pid_b == 0) {
            char shm_str[32], sem_str[32], bee_str[32];
            snprintf(shm_str, sizeof(shm_str), "%d", g_shmid);
            snprintf(sem_str, sizeof(sem_str), "%d", g_semid);
            snprintf(bee_str, sizeof(bee_str), "%d", i);
            execl("./pszczola", "pszczola", shm_str, sem_str, bee_str, (char*)NULL);
            perror("execl pszczola");
            _exit(1);
        }
    }

    /* Czekamy w pętli, aż ktoś naciśnie Ctrl+C (SIGINT) lub SIGTERM */
    while (g_running) {
        pause();
    }

    return 0;
}
