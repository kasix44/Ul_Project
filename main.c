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

    // Przykład użycia funkcji systemowych do stworzenia/logowania do pliku.
    // Tworzymy (lub nadpisujemy) plik "beehive.log".
    int logFile = open("beehive.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (logFile < 0) {
        perror("[MAIN] open(\"beehive.log\")");
        return 1;
    }

    const char* startMsg = "Beehive simulation started.\n";
    ssize_t writeRes = write(logFile, startMsg, strlen(startMsg));
    if (writeRes < 0) {
        perror("[MAIN] write to beehive.log");
        close(logFile);
        return 1;
    }
    if (close(logFile) < 0) {
        perror("[MAIN] close(beehive.log)");
        return 1;
    }

    // Inicjalizacja wspólnych danych ula
    HiveData hive;
    initHiveData(&hive, N, P);

    // Tworzenie wątku królowej
    pthread_t queenThread;
    QueenArgs* queenArgs = (QueenArgs*)malloc(sizeof(QueenArgs));
    if (!queenArgs) {
        perror("[MAIN] malloc queenArgs");
        return 1;
    }
    queenArgs->T_k = T_k;
    queenArgs->eggsCount = eggsCount;
    queenArgs->hive = &hive;

    if (pthread_create(&queenThread, NULL, queenWorker, queenArgs) != 0) {
        perror("[MAIN] pthread_create queen");
        free(queenArgs);
        return 1;
    }

    // Tworzenie wątku pszczelarza
    pthread_t beekeeperThread;
    BeekeeperArgs* keeperArgs = (BeekeeperArgs*)malloc(sizeof(BeekeeperArgs));
    if (!keeperArgs) {
        perror("[MAIN] malloc keeperArgs");
        return 1;
    }
    keeperArgs->hive = &hive;

    if (pthread_create(&beekeeperThread, NULL, beekeeperWorker, keeperArgs) != 0) {
        perror("[MAIN] pthread_create beekeeper");
        free(queenArgs);
        free(keeperArgs);
        return 1;
    }

    // Tworzenie wątków pszczół robotnic
    pthread_t* beeThreads = (pthread_t*)malloc(sizeof(pthread_t) * workerBeesCount);
    if (!beeThreads) {
        perror("[MAIN] malloc beeThreads");
        free(queenArgs);
        free(keeperArgs);
        return 1;
    }

    for (int i = 0; i < workerBeesCount; i++) {
        BeeArgs* b = (BeeArgs*)malloc(sizeof(BeeArgs));
        if (!b) {
            perror("[MAIN] malloc BeeArgs");
            // Awaryjnie możesz zadecydować, czy kończyć program czy nie.
            continue;
        }
        b->id = i;
        b->visits = 0;
        b->maxVisits = 3;  // Po ilu wizytach pszczoła umiera
        b->T_inHive = 2;   // Czas w ulu (sekundy)
        b->hive = &hive;

        // Zwiększamy liczbę żywych pszczół
        if (pthread_mutex_lock(&hive.hiveMutex) != 0) {
            perror("[MAIN] pthread_mutex_lock (beesAlive++)");
            free(b);
            continue;
        }
        hive.beesAlive++;
        if (pthread_mutex_unlock(&hive.hiveMutex) != 0) {
            perror("[MAIN] pthread_mutex_unlock (beesAlive++)");
            free(b);
            continue;
        }

        if (pthread_create(&beeThreads[i], NULL, beeWorker, b) != 0) {
            perror("[MAIN] pthread_create beeWorker");
            free(b);
            // tutaj można zadecydować o kontynuacji lub przerwaniu
        }
    }

    // Czekamy, aż wszystkie wątki pszczół zakończą życie
    // (każda pszczoła wejdzie do ula maxVisits razy i w końcu zakończy wątek).
    for (int i = 0; i < workerBeesCount; i++) {
        pthread_join(beeThreads[i], NULL);
    }
    free(beeThreads);

    // Program ma działać w nieskończoność:
    // - Królowa i Pszczelarz w nieskończonych pętlach (while(1)).
    // - Nie wywołujemy pthread_cancel() tych wątków.
    // - Zamiast tego main czeka na ich zakończenie (co z definicji nie nastąpi).
    // => Program będzie trwał dopóki go nie zabijemy sygnałem z zewnątrz (CTRL+C lub kill).
    pthread_join(queenThread, NULL);
    pthread_join(beekeeperThread, NULL);

    free(queenArgs);
    free(keeperArgs);

    if (pthread_mutex_destroy(&hive.hiveMutex) != 0) {
        perror("[MAIN] pthread_mutex_destroy");
    }

    printf("[MAIN] Koniec symulacji. (Teoretycznie nigdy tu nie dojdziemy)\n");
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

    // Inicjalizacja mutexu
    if (pthread_mutex_init(&hive->hiveMutex, NULL) != 0) {
        perror("[MAIN] pthread_mutex_init(hiveMutex)");
        exit(EXIT_FAILURE);
    }
}
