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
    pthread_t thread;
    Role role;
    int id;
} Request;

Request queue[MAX_QUEUE];
int queue_start = 0, queue_end = 0;

int active_readers = 0;
bool writer_active = false;

int reader_queue_count = 0;
int writer_queue_count = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// wypisywanie aktualnego stanu
void print_status() {
    printf("ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n",
           reader_queue_count,
           writer_queue_count,
           active_readers,
           writer_active ? 1 : 0);
}

// dodanie do kolejki
void enqueue(Request r) {
    queue[queue_end] = r;
    queue_end = (queue_end + 1) % MAX_QUEUE;

    if (r.role == READER) reader_queue_count++;
    else writer_queue_count++;

    print_status();
}

// usunięcie z kolejki
Request dequeue() {
    Request r = queue[queue_start];
    queue_start = (queue_start + 1) % MAX_QUEUE;

    if (r.role == READER) reader_queue_count--;
    else writer_queue_count--;

    print_status();
    return r;
}

// czy wątek jest pierwszy w kolejce
bool is_first_in_queue(pthread_t tid) {
    return queue_start != queue_end && pthread_equal(queue[queue_start].thread, tid);
}

// funkcja czytelnika
void *reader(void *arg) {
    int id = *((int *)arg);
    free(arg);

    while (1) {
        pthread_t tid = pthread_self();
        Request r = {tid, READER, id};

        pthread_mutex_lock(&mutex);
        enqueue(r);

        while (!is_first_in_queue(tid) || writer_active) {
            pthread_cond_wait(&cond, &mutex);
        }

        dequeue();
        active_readers++;
        print_status();
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex);

        printf("Czytelnik %d czyta\n", id);
        sleep(rand() % 3 + 1);

        pthread_mutex_lock(&mutex);
        active_readers--;
        print_status();
        if (active_readers == 0)
            pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex);

        printf("Czytelnik %d kończy czytanie\n", id);
        sleep(rand() % 3 + 1);
    }

    return NULL;
}

// funkcja pisarza
void *writer(void *arg) {
    int id = *((int *)arg);
    free(arg);

    while (1) {
        pthread_t tid = pthread_self();
        Request r = {tid, WRITER, id};

        pthread_mutex_lock(&mutex);
        enqueue(r);

        while (!is_first_in_queue(tid) || active_readers > 0 || writer_active) {
            pthread_cond_wait(&cond, &mutex);
        }

        dequeue();
        writer_active = true;
        print_status();
        pthread_mutex_unlock(&mutex);

        printf("Pisarz %d pisze\n", id);
        sleep(rand() % 3 + 2);

        pthread_mutex_lock(&mutex);
        writer_active = false;
        print_status();
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex);

        printf("Pisarz %d kończy pisanie\n", id);
        sleep(rand() % 3 + 1);
    }

    return NULL;
}

// funkcja główna
int main(int argc, char *argv[]) {
    int num_of_writers;
    int num_of_readers;
    for(int i=1; i<argc; i++){
        if(strcmp(argv[i],"ReaderQ:")==0){
            num_of_readers = atoi(argv[++i]);
            printf("num of readers: %d\n", num_of_readers);
        }else
        if(strcmp(argv[i],"WriterQ:")==0){
            num_of_writers = atoi(argv[++i]);
            printf("num_of_writers: %d\n", num_of_writers);
        }else
    }
    srand(time(NULL));

    pthread_t threads[num_of_readers + num_of_writers];

    // uruchomienie czytelników
    for (int i = 0; i < num_of_readers; i++) {
        int *id = malloc(sizeof(int));
        *id = i;
        pthread_create(&threads[i], NULL, reader, id);
    }

    // uruchomienie pisarzy
    for (int i = 0; i < num_of_writers; i++) {
        int *id = malloc(sizeof(int));
        *id = i;
        pthread_create(&threads[num_of_readers + i], NULL, writer, id);
    }

    // program działa bez końca
    for (int i = 0; i < num_of_readers + num_of_writers; i++) {
        pthread_join(threads[i], NULL);  // w praktyce nigdy się nie kończy
    }

    return 0;
}
