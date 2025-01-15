#include "beekeeper.h"
#include <string.h>

static BeekeeperArgs* gBeekeeperArgs = NULL;

void handleSignalAddFrames(int signum) {
    if (gBeekeeperArgs == NULL) return;
    HiveData* hive = gBeekeeperArgs->hive;

    pthread_mutex_lock(&hive->hiveMutex);
    hive->maxCapacity = 2 * hive->N; // Zwiększamy do 2*N
    printf("[Pszczelarz - sygnał] Dodano ramki. maxCapacity = %d\n", hive->maxCapacity);
    pthread_mutex_unlock(&hive->hiveMutex);
}

void handleSignalRemoveFrames(int signum) {
    if (gBeekeeperArgs == NULL) return;
    HiveData* hive = gBeekeeperArgs->hive;

    pthread_mutex_lock(&hive->hiveMutex);
    hive->maxCapacity /= 2;         // Zmniejszamy o 50%
    if (hive->maxCapacity < hive->P) {
        hive->maxCapacity = hive->P; // Nie może być mniej niż P
    }
    printf("[Pszczelarz - sygnał] Usunięto ramki. maxCapacity = %d\n", hive->maxCapacity);
    pthread_mutex_unlock(&hive->hiveMutex);
}

void* beekeeperWorker(void* arg) {
    gBeekeeperArgs = (BeekeeperArgs*)arg;

    struct sigaction sa1;
    memset(&sa1, 0, sizeof(sa1));
    sa1.sa_handler = handleSignalAddFrames;
    sigaction(SIGUSR1, &sa1, NULL);

    struct sigaction sa2;
    memset(&sa2, 0, sizeof(sa2));
    sa2.sa_handler = handleSignalRemoveFrames;
    sigaction(SIGUSR2, &sa2, NULL);

    while (1) {
        sleep(5);
        printf("[Pszczelarz] Czekam na sygnały (co 5 sek)...\n");
    }

    pthread_exit(NULL);
}
