#include "queen.h"

void* queenWorker(void* arg) {
    QueenArgs* queen = (QueenArgs*)arg;
    HiveData* hive = queen->hive;

    while (1) {
        sleep(queen->T_k); // co T_k sekund

        pthread_mutex_lock(&hive->hiveMutex);
        int wolneMiejsce = hive->maxCapacity - hive->currentBeesInHive;
        if (wolneMiejsce >= queen->eggsCount) {
            printf("[Królowa] Składa %d jaj.\n", queen->eggsCount);
            hive->beesAlive += queen->eggsCount;
            printf("[Królowa] Teraz żywych pszczół: %d\n", hive->beesAlive);
            // Tu można by tworzyć nowe wątki pszczół, ale pomijamy dla uproszczenia
        } else {
            printf("[Królowa] Za mało miejsca w ulu (wolne: %d).\n", wolneMiejsce);
        }
        pthread_mutex_unlock(&hive->hiveMutex);
    }

    pthread_exit(NULL);
}
