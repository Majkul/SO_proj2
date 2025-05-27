#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

#define MAX_QUEUE 100

typedef enum { READER } Role;

int active_readers = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void print_status() {
    printf("[Stan] Czytelników w środku: %d\n", active_readers);
}

// Funkcja czytelnika W KOLEJCE (czyli przez FIFO)
void* reader_thread(void* arg) {
    int id = *((int*)arg);
    free(arg);

    pthread_mutex_lock(&mutex);
    while (active_readers > 0) {
        pthread_cond_wait(&cond, &mutex);
    }

    active_readers++;
    printf("Czytelnik %d wchodzi do czytelni z kolejki.\n", id);
    print_status();
    pthread_mutex_unlock(&mutex);

    sleep(rand() % 3 + 2);  // czytanie

    pthread_mutex_lock(&mutex);
    printf("Czytelnik %d wychodzi z czytelni.\n", id);
    active_readers--;
    print_status();
    pthread_cond_broadcast(&cond);  // może obudzić następnych
    pthread_mutex_unlock(&mutex);

    return NULL;
}

// Funkcja czytelnika, który od razu jest w środku
void* initial_reader(void* arg) {
    int id = *((int*)arg);
    free(arg);

    printf("Czytelnik %d już jest w środku.\n", id);
    sleep(rand() % 5 + 2);  // czas czytania

    pthread_mutex_lock(&mutex);
    printf("Czytelnik %d wychodzi z czytelni.\n", id);
    active_readers--;
    print_status();
    pthread_cond_broadcast(&cond);  // budzi tych w kolejce
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main() {
    srand(time(NULL));

    int czytelnicy_w_srodku = 3;
    int czytelnicy_w_kolejce = 2;

    pthread_t threads[czytelnicy_w_srodku + czytelnicy_w_kolejce];

    // Czytelnicy już w środku
    for (int i = 0; i < czytelnicy_w_srodku; i++) {
        pthread_mutex_lock(&mutex);
        active_readers++;
        print_status();
        pthread_mutex_unlock(&mutex);

        int* id = malloc(sizeof(int));
        *id = i;
        pthread_create(&threads[i], NULL, initial_reader, id);
    }

    sleep(1);  // dajmy im chwilę na „wejście”

    // Czytelnicy w kolejce
    for (int i = 0; i < czytelnicy_w_kolejce; i++) {
        int* id = malloc(sizeof(int));
        *id = i + czytelnicy_w_srodku;
        pthread_create(&threads[czytelnicy_w_srodku + i], NULL, reader_thread, id);
    }

    for (int i = 0; i < czytelnicy_w_srodku + czytelnicy_w_kolejce; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
