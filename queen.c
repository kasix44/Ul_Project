#include "queen.h"

void* queenWorker(void* arg) {
    QueenArgs* queen = (QueenArgs*)arg;
    HiveData* hive = queen->hive;

    while (1) {
        sleep(queen->T_k); // co T_k sekund

        if (pthread_mutex_lock(&hive->hiveMutex) != 0) {
            perror("[Królowa] pthread_mutex_lock");
            break; // przerywamy pętlę (lub pthread_exit(NULL))
        }

        int wolneMiejsce = hive->maxCapacity - hive->currentBeesInHive;
        if (wolneMiejsce >= queen->eggsCount) {
            printf("[Królowa] Składa %d jaj.\n", queen->eggsCount);
            hive->beesAlive += queen->eggsCount;
            printf("[Królowa] Teraz żywych pszczół: %d\n", hive->beesAlive);
            
        } else {
            printf("[Królowa] Za mało miejsca w ulu (wolne: %d).\n", wolneMiejsce);
        }

        if (pthread_mutex_unlock(&hive->hiveMutex) != 0) {
            perror("[Królowa] pthread_mutex_unlock");
            break;
        }
    }

    // W tej wersji teoretycznie nie kończymy, pętla while(1) jest nieskończona,
    // więc w praktyce wątek trwa do końca programu.
    pthread_exit(NULL);
}
