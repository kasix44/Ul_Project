#include "common.h"
#include "bee.h"
#include "queen.h"
#include "beekeeper.h"

void initHiveData(HiveData* hive, int N, int P);

int main(int argc, char* argv[]) {
    if (argc < 6) {
        fprintf(stderr, "Użycie: %s <N> <P> <T_k> <eggsCount> <liczba_pszczol_robotnic>\n", argv[0]);
        return 1;
    }
    int N = atoi(argv[1]);
    int P = atoi(argv[2]);
    int T_k = atoi(argv[3]);
    int eggsCount = atoi(argv[4]);
    int workerBeesCount = atoi(argv[5]);

    if (N <= 0 || P <= 0 || P >= N/2 || T_k <= 0 || eggsCount <= 0 || workerBeesCount <= 0) {
        fprintf(stderr, "Błędne wartości. Upewnij się, że P < N/2 i wszystko > 0.\n");
        return 1;
    }

    HiveData hive;
    initHiveData(&hive, N, P);

    // Tworzenie wątku królowej
    pthread_t queenThread;
    QueenArgs* queenArgs = malloc(sizeof(QueenArgs));
    queenArgs->T_k = T_k;
    queenArgs->eggsCount = eggsCount;
    queenArgs->hive = &hive;
    if (pthread_create(&queenThread, NULL, queenWorker, queenArgs) != 0) {
        perror("pthread_create queen");
        return 1;
    }

    // Tworzenie wątku pszczelarza
    pthread_t beekeeperThread;
    BeekeeperArgs* keeperArgs = malloc(sizeof(BeekeeperArgs));
    keeperArgs->hive = &hive;
    if (pthread_create(&beekeeperThread, NULL, beekeeperWorker, keeperArgs) != 0) {
        perror("pthread_create beekeeper");
        return 1;
    }

    // Tworzenie wątków pszczół robotnic
    pthread_t* beeThreads = malloc(sizeof(pthread_t) * workerBeesCount);
    for (int i = 0; i < workerBeesCount; i++) {
        BeeArgs* b = malloc(sizeof(BeeArgs));
        b->id = i;
        b->visits = 0;
        b->maxVisits = 3;  // Po ilu wizytach pszczoła umiera
        b->T_inHive = 2;   // Czas w ulu
        b->hive = &hive;

        pthread_mutex_lock(&hive.hiveMutex);
        hive.beesAlive++;
        pthread_mutex_unlock(&hive.hiveMutex);

        if (pthread_create(&beeThreads[i], NULL, beeWorker, b) != 0) {
            perror("pthread_create bee");
            return 1;
        }
    }

    // Czekamy na zakończenie wątków pszczół
    for (int i = 0; i < workerBeesCount; i++) {
        pthread_join(beeThreads[i], NULL);
    }
    free(beeThreads);

    // Zatrzymujemy królową i pszczelarza (teoretycznie bezterminowo działają)
    pthread_cancel(queenThread);
    pthread_cancel(beekeeperThread);
    pthread_join(queenThread, NULL);
    pthread_join(beekeeperThread, NULL);

    free(queenArgs);
    free(keeperArgs);
    pthread_mutex_destroy(&hive.hiveMutex);

    printf("[MAIN] Koniec symulacji.\n");
    return 0;
}

void initHiveData(HiveData* hive, int N, int P) {
    hive->currentBeesInHive = 0;
    hive->maxCapacity = P;
    hive->N = N;
    hive->P = P;
    hive->entranceInUse[0] = false;
    hive->entranceInUse[1] = false;
    hive->beesAlive = 0;

    if (pthread_mutex_init(&hive->hiveMutex, NULL) != 0) {
        perror("pthread_mutex_init");
        exit(EXIT_FAILURE);
    }
}
