#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>

const int reading_time = 100;
const int writing_time = 5000000;
const int readers_waiting_time = 50;
const int writers_waiting_time = 500;

volatile int n_readers = 20;
volatile int n_writers = 10;

typedef struct {
    int writing;
    int reading;
    int writing_queue;
    int reading_queue;
    pthread_mutex_t mutex;
    pthread_cond_t reading_cond;
    pthread_cond_t writing_cond;
} Monitor;

void monitor_init(Monitor* m) {
    m->writing = 0;
    m->reading = 0;
    pthread_mutex_init(&m->mutex, NULL);
    pthread_cond_init(&m->reading_cond, NULL);
    pthread_cond_init(&m->writing_cond, NULL);
}

Monitor Czytelnia;

void printinfo() {
    printf("ReaderQ: %d, WriterQ: %d, [in R: %d, W: %d]\n", Czytelnia.reading_queue, Czytelnia.writing_queue, Czytelnia.reading, Czytelnia.writing);
}

void* reading(void* i) {
    while(1) {
        pthread_mutex_lock(&Czytelnia.mutex);
        printf("Reader %ld is waiting to read\n", (intptr_t)i);
        Czytelnia.reading_queue++;
        printinfo();
        while (Czytelnia.writing > 0) {
            pthread_cond_wait(&Czytelnia.reading_cond, &Czytelnia.mutex);
        }
        printf("Reader %ld is reading\n", (intptr_t)i);
        Czytelnia.reading++;
        Czytelnia.reading_queue--;
        printinfo();
        pthread_mutex_unlock(&Czytelnia.mutex);

        usleep(reading_time);

        pthread_mutex_lock(&Czytelnia.mutex);
        printf("Reader %ld finished reading\n", (intptr_t)i);
        Czytelnia.reading--;
        printinfo();
        if (Czytelnia.reading == 0) {
            pthread_cond_signal(&Czytelnia.writing_cond);
        }
        pthread_mutex_unlock(&Czytelnia.mutex);

        usleep(readers_waiting_time);
    }
}

void* writing(void* i) {
    while(1) {
        pthread_mutex_lock(&Czytelnia.mutex);
        printf("Writer %ld is waiting to write\n", (intptr_t)i);
        Czytelnia.writing_queue++;
        printinfo();
        while (Czytelnia.reading + Czytelnia.writing > 0) {
            pthread_cond_wait(&Czytelnia.writing_cond, &Czytelnia.mutex);
        }
        printf("Writer %ld is writing\n", (intptr_t)i);
        Czytelnia.writing = 1;
        Czytelnia.writing_queue--;
        printinfo();
        pthread_mutex_unlock(&Czytelnia.mutex);

        usleep(writing_time);

        pthread_mutex_lock(&Czytelnia.mutex);
        printf("Writer %ld finished writing\n", (intptr_t)i);
        Czytelnia.writing = 0;
        printinfo();
        pthread_cond_broadcast(&Czytelnia.reading_cond);
        pthread_mutex_unlock(&Czytelnia.mutex);

        usleep(writers_waiting_time);
    }
}

int main() {
    
    // int n_readers, nwriters;

    pthread_t readers[n_readers], writers[n_writers];
    monitor_init(&Czytelnia);

    printinfo();
    
    for (int i = 0; i < n_readers; i++) {
        pthread_create(&readers[i], NULL, reading, (void*)(intptr_t)i);
    }
    for (int i = 0; i < n_writers; i++) {
        pthread_create(&writers[i], NULL, writing, (void*)(intptr_t)i);
    }

    for (int i = 0; i < n_readers; i++) {
        pthread_join(readers[i], NULL);
    }
    for (int i = 0; i < n_writers; i++) {
        pthread_join(writers[i], NULL);
    }

    return 0;
}