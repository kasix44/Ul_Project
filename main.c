#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>

#include "hive.h"

// Zmienne globalne do obsługi sygnałów
volatile sig_atomic_t running = 1;
pid_t pid_beekeeper, pid_queen;
int shmid = -1, semid = -1;
hive_t *hive = NULL;

// Handler sygnałów
void signal_handler(int signo) {
    running = 0;
}

// Funkcja czyszcząca
void cleanup_all(void) {
    printf("\n[MAIN] Rozpoczynam zamykanie programu...\n");

    // Wysyłamy SIGTERM do pszczelarza i królowej
    if (pid_beekeeper > 0) kill(pid_beekeeper, SIGTERM);
    if (pid_queen > 0) kill(pid_queen, SIGTERM);

    // Czekamy na zakończenie wszystkich procesów potomnych
    while (wait(NULL) > 0) {
        // zbieramy trupy
    }

    // Sprzątanie zasobów systemowych
    if (hive != NULL && shmid != -1 && semid != -1) {
        cleanup_system(shmid, semid, hive);
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

    // Rejestracja handlera sygnałów
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

     if(sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    // Rejestracja funkcji czyszczącej
    if (atexit(cleanup_all) != 0) {
        fprintf(stderr, "Nie można zarejestrować funkcji atexit\n");
        exit(EXIT_FAILURE);
    }

    if (init_system(N, P, Tk, &shmid, &semid, &hive) < 0) {
        fprintf(stderr, "[MAIN] init_system error\n");
        exit(EXIT_FAILURE);
    }

    /* Tworzymy proces pszczelarza */
    pid_beekeeper = fork();
    if (pid_beekeeper < 0) {
        perror("fork beekeeper");
        exit(EXIT_FAILURE);
    } else if (pid_beekeeper == 0) {
        beekeeper_proc(hive, semid);
        _exit(0);
    }

    /* Tworzymy proces królowej */
    pid_queen = fork();
    if (pid_queen < 0) {
        perror("fork queen");
        kill(pid_beekeeper, SIGTERM);
        exit(EXIT_FAILURE);
    } else if (pid_queen == 0) {
        queen_proc(hive, semid);
        _exit(0);
    }

    /* Tworzymy N pszczół (robotnice) */
    for (int i = 0; i < N; i++) {
        pid_t pid_b = fork();
        if (pid_b < 0) {
            perror("fork bee");
            exit(EXIT_FAILURE);
        }
        if (pid_b == 0) {
            bee_proc(hive, semid, i);
            _exit(0);
        }
    }

    // Pętla główna programu
    while (running) {
        pause(); // Czekamy na sygnał
    }

    return 0;
}
