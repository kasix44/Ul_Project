#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>

#include "hive.h"

/* Deklaracje "procedur" (z plików .c) */
extern void beekeeper_proc(hive_t *hive, int semid);
extern void queen_proc(hive_t *hive, int semid);
extern void bee_proc(hive_t *hive, int semid, int bee_id);

/* Zmienne globalne (tylko dla głównego trybu) */
static volatile sig_atomic_t running = 1;
static pid_t pid_beekeeper = -1, pid_queen = -1;
static int shmid = -1, semid_ = -1;
static hive_t *hive = NULL;

/* Handler SIGINT / SIGTERM */
static void signal_handler(int signo) {
    (void)signo;
    running = 0;
}

/* Funkcja czyszcząca – tylko w głównym trybie */
static void cleanup_all(void) {
    printf("\n[MAIN] Rozpoczynam zamykanie programu...\n");

    if (pid_beekeeper > 0) kill(pid_beekeeper, SIGTERM);
    if (pid_queen > 0)     kill(pid_queen, SIGTERM);

    while (waitpid(-1, NULL, 0) > 0) {
        /* zbieramy trupy */
    }

    if (hive != NULL && shmid != -1 && semid_ != -1) {
        cleanup_system(shmid, semid_, hive);
    }

    printf("[MAIN] Zakończono.\n");
}

/* Uruchomienie w roli głównego programu (przykładowo "./ul 5 2 3") */
static int run_as_main(int argc, char *argv[])
{
    if (argc < 4) {
        fprintf(stderr, "Użycie: %s <N> <P> <Tk>\n", argv[0]);
        return 1;
    }

    int N  = atoi(argv[1]);
    int P  = atoi(argv[2]);
    int Tk = atoi(argv[3]);

    if (N < 1 || P < 1 || Tk < 1) {
        fprintf(stderr, "Błędne parametry - wszystkie muszą być > 0.\n");
        return 1;
    }
    if (P >= (N/2)) {
        fprintf(stderr, "Warunek P < N/2 nie jest spełniony!\n");
        return 1;
    }
    if (N > MAX_BEES) {
        fprintf(stderr, "N zbyt duże (max %d)\n", MAX_BEES);
        return 1;
    }

    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction SIGTERM");
        return 1;
    }
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction SIGINT");
        return 1;
    }

    if (atexit(cleanup_all) != 0) {
        fprintf(stderr, "Nie można zarejestrować funkcji atexit\n");
        return 1;
    }

    if (init_system(N, P, Tk, &shmid, &semid_, &hive) < 0) {
        fprintf(stderr, "[MAIN] init_system error\n");
        return 1;
    }

    /* Uruchamiamy pszczelarza (argv[0]="pszczelarz") */
    pid_beekeeper = fork();
    if (pid_beekeeper < 0) {
        perror("fork beekeeper");
        return 1;
    } else if (pid_beekeeper == 0) {
        char shm_str[32], sem_str[32];
        snprintf(shm_str, sizeof(shm_str), "%d", shmid);
        snprintf(sem_str, sizeof(sem_str), "%d", semid_);
        execl("./ul", "pszczelarz", shm_str, sem_str, NULL);
        perror("execl pszczelarz");
        _exit(1);
    }

    /* Uruchamiamy królową (argv[0]="krolowa") */
    pid_queen = fork();
    if (pid_queen < 0) {
        perror("fork queen");
        kill(pid_beekeeper, SIGTERM);
        return 1;
    } else if (pid_queen == 0) {
        char shm_str[32], sem_str[32];
        snprintf(shm_str, sizeof(shm_str), "%d", shmid);
        snprintf(sem_str, sizeof(sem_str), "%d", semid_);
        execl("./ul", "krolowa", shm_str, sem_str, NULL);
        perror("execl krolowa");
        _exit(1);
    }

    /* Tworzymy N pszczół (argv[0]="pszczola") */
    for (int i = 0; i < N; i++) {
        pid_t pid_b = fork();
        if (pid_b < 0) {
            perror("fork bee");
            return 1;
        }
        if (pid_b == 0) {
            char shm_str[32], sem_str[32], bee_str[32];
            snprintf(shm_str, sizeof(shm_str), "%d", shmid);
            snprintf(sem_str, sizeof(sem_str), "%d", semid_);
            snprintf(bee_str, sizeof(bee_str), "%d", i);
            execl("./ul", "pszczola", shm_str, sem_str, bee_str, NULL);
            perror("execl pszczola");
            _exit(1);
        }
    }

    while (running) {
        pause();
    }

    return 0;
}

/* Jeden jedyny main */
int main(int argc, char *argv[])
{
    /* Rozpoznajemy, jak uruchomiony został proces (argv[0]) */
    if (strcmp(argv[0], "pszczelarz") == 0) {
        // tryb "pszczelarz"
        if (argc < 3) {
            fprintf(stderr, "[PSZCZELARZ] Brak arg: <shmid> <semid>\n");
            exit(EXIT_FAILURE);
        }
        int shmid_local = atoi(argv[1]);
        int semid_local = atoi(argv[2]);

        hive_t *hive_local = (hive_t*) shmat(shmid_local, NULL, 0);
        if (hive_local == (void*)-1) {
            perror("shmat pszczelarz");
            exit(EXIT_FAILURE);
        }

        beekeeper_proc(hive_local, semid_local);
        shmdt(hive_local);
        return 0;

    } else if (strcmp(argv[0], "krolowa") == 0) {
        // tryb "krolowa"
        if (argc < 3) {
            fprintf(stderr, "[KROLOWA] Brak arg: <shmid> <semid>\n");
            exit(EXIT_FAILURE);
        }
        int shmid_local = atoi(argv[1]);
        int semid_local = atoi(argv[2]);

        hive_t *hive_local = (hive_t*) shmat(shmid_local, NULL, 0);
        if (hive_local == (void*)-1) {
            perror("shmat krolowa");
            exit(EXIT_FAILURE);
        }

        queen_proc(hive_local, semid_local);
        shmdt(hive_local);
        return 0;

    } else if (strcmp(argv[0], "pszczola") == 0) {
        // tryb "pszczola"
        if (argc < 4) {
            fprintf(stderr, "[PSZCZOLA] Brak arg: <shmid> <semid> <bee_id>\n");
            exit(EXIT_FAILURE);
        }
        int shmid_local = atoi(argv[1]);
        int semid_local = atoi(argv[2]);
        int bee_id      = atoi(argv[3]);

        hive_t *hive_local = (hive_t*) shmat(shmid_local, NULL, 0);
        if (hive_local == (void*)-1) {
            perror("shmat pszczola");
            exit(EXIT_FAILURE);
        }

        bee_proc(hive_local, semid_local, bee_id);
        shmdt(hive_local);
        return 0;

    } else {
        // tryb główny (np. ./ul N P Tk)
        return run_as_main(argc, argv);
    }
}
