#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

#define MAX_QUEUE 100

typedef enum {READER, WRITER} Role;

typedef struct {
    pthread_t thread;
    Role role;
    int id;
} Request;

Request queue[MAX_QUEUE];
int queue_start = 0, queue_end = 0;

int active_readers = 0;
bool writer_active = false;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// Dodanie do kolejki
void enqueue(Request r) {
    queue[queue_end] = r;
    queue_end = (queue_end + 1) % MAX_QUEUE;
}

// Usunięcie z kolejki
Request dequeue() {
    Request r = queue[queue_start];
    queue_start = (queue_start + 1) % MAX_QUEUE;
    return r;
}

// Sprawdzenie, czy wątek jest pierwszy w kolejce
bool is_first_in_queue(pthread_t tid) {
    return queue_start != queue_end && pthread_equal(queue[queue_start].thread, tid);
}

// Funkcja czytelnika
void *reader(void *arg) {
    int id = *((int *)arg);
    free(arg);
    pthread_t tid = pthread_self();

    Request r = {tid, READER, id};

    pthread_mutex_lock(&mutex);
    enqueue(r);

    while (!is_first_in_queue(tid) || writer_active) {
        pthread_cond_wait(&cond, &mutex);
    }

    dequeue();
    active_readers++;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);

    printf("Czytelnik %d czyta\n", id);
    sleep(rand() % 3 + 1); // symulacja czytania

    pthread_mutex_lock(&mutex);
    active_readers--;
    if (active_readers == 0)
        pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);

    printf("Czytelnik %d kończy czytanie\n", id);
    return NULL;
}

// Funkcja pisarza
void *writer(void *arg) {
    int id = *((int *)arg);
    free(arg);
    pthread_t tid = pthread_self();

    Request r = {tid, WRITER, id};

    pthread_mutex_lock(&mutex);
    enqueue(r);

    while (!is_first_in_queue(tid) || active_readers > 0 || writer_active) {
        pthread_cond_wait(&cond, &mutex);
    }

    dequeue();
    writer_active = true;
    pthread_mutex_unlock(&mutex);

    printf("Pisarz %d pisze\n", id);
    sleep(rand() % 3 + 2); // symulacja pisania

    pthread_mutex_lock(&mutex);
    writer_active = false;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);

    printf("Pisarz %d kończy pisanie\n", id);
    return NULL;
}

// Funkcja główna
int main() {
    srand(time(NULL));
    pthread_t threads[20];

    for (int i = 0; i < 20; i++) {
        int *id = malloc(sizeof(int));
        *id = i;

        if (rand() % 2 == 0)
            pthread_create(&threads[i], NULL, reader, id);
        else
            pthread_create(&threads[i], NULL, writer, id);

        usleep(100000); // opóźnienie, by wątki dodawały się kolejno
    }

    for (int i = 0; i < 20; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
