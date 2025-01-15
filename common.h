#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

/*
  Struktura z danymi o ulu i globalnym stanie.
  Obsługa błędów: używamy perror() i errno w kluczowych miejscach (np. przy funkcjach systemowych).
*/

#define MAX_BEES 1000

typedef struct {
    int currentBeesInHive;  // Aktualna liczba pszczół w ulu
    int maxCapacity;        // Aktualna pojemność ula
    int N;                  // Początkowa liczba pszczół (rozmiar roju)
    int P;                  // Początkowy limit pszczół w ulu
    bool entranceInUse[2];  // Czy wejście [0] lub [1] jest zajęte

    pthread_mutex_t hiveMutex; // Mutex do synchronizacji

    int beesAlive;         // Ilość żywych pszczół (robotnice + ewentualnie nowe)
} HiveData;

#endif
