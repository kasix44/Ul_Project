#ifndef HIVE_H
#define HIVE_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <errno.h>

/* Stale, np. do ograniczenia maksymalnej liczby pszczół */
#define MAX_BEES 200
#define MAX_VISITS 10  /* pszczoła może odwiedzić ul max X_i razy */
#define SEM_COUNT 6    /* liczba semaforów w zestawie */

/* Numery semaforów w tablicy */
enum {
    SEM_MUTEX = 0,     /* ochrona pamięci (sekcja krytyczna) */
    SEM_CAP,           /* ograniczenie liczby pszczół w ulu */
    SEM_ENT0_IN,       /* wejście 0, ruch inbound  */
    SEM_ENT0_OUT,      /* wejście 0, ruch outbound */
    SEM_ENT1_IN,       /* wejście 1, ruch inbound  */
    SEM_ENT1_OUT       /* wejście 1, ruch outbound */
};

/* Struktura przechowywana w pamięci dzielonej */
typedef struct {
    int N;               /* początkowa liczba pszczół                 */
    int P;               /* początkowa pojemność (P < N/2)            */
    int current_inside;  /* aktualna liczba pszczół w ulu             */
    int max_capacity;    /* bieżąca maksymalna pojemność ula          */

    int total_bees;      /* ile w sumie stworzono pszczół (żywych + martwych) */
    int living_bees;     /* liczba aktualnie żywych pszczół           */

    int queen_interval;  /* co ile sekund królowa składa jaja (Tk)    */

    /* Dwa wejścia, każde może być: 0=idle, 1=inbound, 2=outbound */
    int direction[2];
    int count[2];          /* ile pszczół aktualnie korzysta z danego wejścia */
    int wait_inbound[2];   /* ilu czeka w kolejce na inbound  */
    int wait_outbound[2];  /* ilu czeka w kolejce na outbound */

} hive_t;

/* Funkcje z hive.c */
int  init_system(int N, int P, int T_k, int *shmid_out, int *semid_out, hive_t **ptr_out);
void cleanup_system(int shmid, int semid, hive_t *ptr);

/* Funkcje z sync.c */
void sem_down(int semid, int sem_num);
void sem_up(int semid, int sem_num);

/* Funkcje z pszczelarz.c */
void beekeeper_proc(hive_t *hive, int semid);

/* Funkcje z krolowa.c */
void queen_proc(hive_t *hive, int semid);

/* Funkcje z pszczola.c */
void bee_proc(hive_t *hive, int semid, int bee_id);
void enter_hive(hive_t *hive, int semid);
void exit_hive(hive_t *hive, int semid);

#endif
