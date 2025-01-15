#include "bee.h"

void* beeWorker(void* arg) {
    BeeArgs* bee = (BeeArgs*)arg;
    HiveData* hive = bee->hive;

    srand(time(NULL) + bee->id);

    while (bee->visits < bee->maxVisits) {
        int entrance = rand() % 2;  // Wybór jednego z dwóch wejść

        // Próba wejścia do ula
        pthread_mutex_lock(&hive->hiveMutex);

        while (hive->entranceInUse[entrance] == true ||
               hive->currentBeesInHive >= hive->maxCapacity) {
            pthread_mutex_unlock(&hive->hiveMutex);
            usleep(100000); // 0.1s przerwy
            pthread_mutex_lock(&hive->hiveMutex);
        }
        // Zajmujemy wejście
        hive->entranceInUse[entrance] = true;
        hive->currentBeesInHive++;
        printf("[Pszczoła %d] Wchodzi przez wejście %d. (W ulu: %d)\n",
               bee->id, entrance, hive->currentBeesInHive);
        pthread_mutex_unlock(&hive->hiveMutex);

        // Przebywanie w ulu
        sleep(bee->T_inHive);

        // Wyjście z ula
        pthread_mutex_lock(&hive->hiveMutex);
        hive->currentBeesInHive--;
        hive->entranceInUse[entrance] = false;
        printf("[Pszczoła %d] Wychodzi przez wejście %d. (W ulu: %d)\n",
               bee->id, entrance, hive->currentBeesInHive);
        pthread_mutex_unlock(&hive->hiveMutex);

        bee->visits++;
        // Czas spędzony poza ulem
        int outsideTime = (rand() % 3) + 1;
        sleep(outsideTime);
    }

    pthread_mutex_lock(&hive->hiveMutex);
    hive->beesAlive--;
    printf("[Pszczoła %d] Umiera. (Pozostało pszczół: %d)\n", bee->id, hive->beesAlive);
    pthread_mutex_unlock(&hive->hiveMutex);

    free(bee); // Zwolnienie pamięci BeeArgs
    pthread_exit(NULL);
}
