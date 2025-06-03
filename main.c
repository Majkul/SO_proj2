#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#define MAX_QUEUE 100

typedef enum {READER, WRITER} Role;

typedef struct {
    pthread_t thread; //id wątku
    Role role; //czytelnik czy pisarz
    int id;
} Request;

typedef struct {
    Request queue[MAX_QUEUE]; //kolejka żądań
    int queue_start; //indeks początku kolejki
    int queue_end; //indeks końca kolejki

    int active_readers; //liczba aktywnych czytelników
    bool writer_active; //czy jest aktualnie pisarz

    int reader_queue_count; //liczba czytelników w kolejce
    int writer_queue_count; //liczba pisarzy w kolejce

    pthread_mutex_t mutex;
    pthread_cond_t cond; //warunek synchronizacji
} Monitor;

//struktura do przekazania argumentów do funkcji wątku
typedef struct {
    int id;
    Monitor *monitor;
} ThreadArgs;

//wypisanie aktualnego stanu
void print_status(Monitor *m) {
    printf("ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n",
           m->reader_queue_count,
           m->writer_queue_count,
           m->active_readers,
           m->writer_active ? 1 : 0);
}

void enqueue(Monitor *m, Request r) {
    m->queue[m->queue_end] = r; //dodanei żądania na koniec kolejki
    m->queue_end = (m->queue_end + 1) % MAX_QUEUE; //zmiana indeksu końca kolejki

    if (r.role == READER) m->reader_queue_count++;
    else m->writer_queue_count++;

    print_status(m);
}

Request dequeue(Monitor *m) {
    Request r = m->queue[m->queue_start]; //odczytanie żądania z początku kolejki
    m->queue_start = (m->queue_start + 1) % MAX_QUEUE; //zmiana indeksu początku kolejki

    if (r.role == READER) m->reader_queue_count--;
    else m->writer_queue_count--;

    print_status(m);
    return r; //zwrócenie odczytanego żądania
}

//sprawdzenie, czy obecny wątek jest pierwszy w kolejce
bool is_first_in_queue(Monitor *m, pthread_t tid) {
    return m->queue_start != m->queue_end &&
           pthread_equal(m->queue[m->queue_start].thread, tid);
}

void *reader(void *arg) {
    Monitor *m = (Monitor *)arg;

    while (1) {
        pthread_t tid = pthread_self(); //id bieżącego wątku
        Request r = {tid, READER};

        pthread_mutex_lock(&m->mutex);
        enqueue(m, r);

        //czeka dopóki nie jest pierwszy w kolejce lub ktoś aktualnie pisze
        while (!is_first_in_queue(m, tid) || m->writer_active) {
            pthread_cond_wait(&m->cond, &m->mutex);
        }

        dequeue(m); //usuwa siebie z kolejki
        m->active_readers++; //zwiększ licznik aktywnych czytelników
        print_status(m);
        pthread_cond_broadcast(&m->cond); //obudź innych (inni czytelnicy mogą wejść)
        pthread_mutex_unlock(&m->mutex);

        sleep(rand() % 3 + 1); //symulacja czasu czytania

        pthread_mutex_lock(&m->mutex);
        m->active_readers--; //zakończ czytanie
        print_status(m);
        if (m->active_readers == 0)
            pthread_cond_broadcast(&m->cond); //jeśli ostatni (nie ma więcej czytelników) to obudźi pisarzy
        pthread_mutex_unlock(&m->mutex);
    }

    return NULL;
}

void *writer(void *arg) {
    Monitor *m = (Monitor *)arg;

    while (1) {
        pthread_t tid = pthread_self(); //id bieżącego wątku
        Request r = {tid, WRITER};

        pthread_mutex_lock(&m->mutex);
        enqueue(m, r);

        //czeka dopóki nie jest pierwszy, są aktywni czytelnicy lub ktoś pisze
        while (!is_first_in_queue(m, tid) || m->active_readers > 0 || m->writer_active) {
            pthread_cond_wait(&m->cond, &m->mutex);
        }

        dequeue(m);//usuwa siebie z kolejki
        m->writer_active = true;
        print_status(m);
        pthread_mutex_unlock(&m->mutex);

        sleep(rand() % 3 + 2); //symulacja czasu pisania

        pthread_mutex_lock(&m->mutex);
        m->writer_active = false;
        print_status(m);
        pthread_cond_broadcast(&m->cond); //obudź czekających
        pthread_mutex_unlock(&m->mutex);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    int num_of_readers = 0;
    int num_of_writers = 0;
    int opt;
    // ./program -r num_of_readers -w num_of_writers
    while ((opt = getopt(argc, argv, "r:w:")) != -1) {
        switch (opt) {
            case 'r':
                num_of_readers = atoi(optarg);
                break;
            case 'w':
                num_of_writers = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Użycie: %s -r liczba_czytelników -w liczba_pisarzy\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    srand(time(NULL));

    pthread_t threads[num_of_readers + num_of_writers];

    //inicjalizacja monitora
    Monitor monitor = {
        .queue_start = 0,
        .queue_end = 0,
        .active_readers = 0,
        .writer_active = false,
        .reader_queue_count = 0,
        .writer_queue_count = 0,
        .mutex = PTHREAD_MUTEX_INITIALIZER,
        .cond = PTHREAD_COND_INITIALIZER
    };

    //tworzenie wątków czytelników
    for (int i = 0; i < num_of_readers; i++) {
        pthread_create(&threads[i], NULL, reader, &monitor);
    }

    //tworzenie wątków pisarzy
    for (int i = 0; i < num_of_writers; i++) {
        pthread_create(&threads[num_of_readers + i], NULL, writer, &monitor);
    }

    //dołączenie wątków
    for (int i = 0; i < num_of_readers + num_of_writers; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
