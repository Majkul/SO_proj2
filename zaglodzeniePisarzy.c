#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>

// Czasy czytania, pisania, oraz czekania pisarzy i czytelników w mikrosekundach 
const int reading_time = 50;
const int writing_time = 50000;
const int readers_waiting_time = 50;
const int writers_waiting_time = 500;

// Monitor do synchronizacji czytelników i pisarzy
typedef struct {
    int is_writing;                 // flaga wskazująca, czy pisarz pisze
    int reading_count;              // liczba aktywnych czytelników
    int writing_queue_count;        // liczba pisarzy w kolejce
    int reading_queue_count;        // liczba czytelników w kolejce
    pthread_mutex_t mutex;          // mutex do synchronizacji dostępu do monitora
    pthread_cond_t reading_cond;    // warunek do oczekiwania czytelników
    pthread_cond_t writing_cond;    // warunek do oczekiwania pisarzy
} Monitor;

// Inicjalizacja monitora
void monitor_init(Monitor* m) {
    // Inicjalizacja pól monitora
    m->is_writing = 0;
    m->reading_count = 0;
    m->writing_queue_count = 0;
    m->reading_queue_count = 0;

    // Inicjalizacja mutexów i warunków
    pthread_mutex_init(&m->mutex, NULL);
    pthread_cond_init(&m->reading_cond, NULL);
    pthread_cond_init(&m->writing_cond, NULL);
}

// Funkcja do wypisywania informacji o stanie czytelni
void print_status(Monitor* m) {
    printf("ReaderQ: %d, WriterQ: %d, [in R: %d, W: %d]\n",
        m->reading_queue_count,
        m->writing_queue_count,
        m->reading_count,
        m->is_writing);
}

// Funkcja wykonywana przez czytelników
void* reading(void* arg) {
    Monitor* m = (Monitor*)arg;
    while(1) {

        // Czytelnik próbuje uzyskać dostęp do czytelni
        pthread_mutex_lock(&m->mutex);
        m->reading_queue_count++;
        print_status(m);

        // Czekaj, aż nie będzie aktywnych pisarzy
        while (m->is_writing > 0) {
            pthread_cond_wait(&m->reading_cond, &m->mutex);
        }
        
        // Czytelnik uzyskał dostęp do czytelni
        m->reading_count++;
        m->reading_queue_count--;
        print_status(m);
        pthread_mutex_unlock(&m->mutex);

        // Czytelnik czyta przez określony czas
        usleep(reading_time);

        // Czytelnik kończy czytanie
        pthread_mutex_lock(&m->mutex);
        m->reading_count--;
        print_status(m);

        // Jeśli nie ma więcej czytelników, obudź pisarzy
        if (m->reading_count == 0) {
            pthread_cond_signal(&m->writing_cond);
        }
        pthread_mutex_unlock(&m->mutex);

        // Czytelnik czeka przed kolejnym czytaniem
        usleep(readers_waiting_time);
    }
}

// Funkcja wykonywana przez pisarzy
void* writing(void* arg) {
    Monitor* m = (Monitor*)arg;
    while(1) {

        // Pisarz próbuje uzyskać dostęp do czytelni
        pthread_mutex_lock(&m->mutex);
        m->writing_queue_count++;
        print_status(m);

        // Czekaj, aż nie będzie aktywnych czytelników i pisarzy
        while (m->reading_count > 0 || m->is_writing) {
            pthread_cond_wait(&m->writing_cond, &m->mutex);
        }

        // Pisarz uzyskał dostęp do czytelni
        m->is_writing = 1;
        m->writing_queue_count--;
        print_status(m);
        pthread_mutex_unlock(&m->mutex);

        // Pisarz pisze przez określony czas
        usleep(writing_time);

        // Pisarz kończy pisanie
        pthread_mutex_lock(&m->mutex);
        m->is_writing = 0;
        print_status(m);

        // Obudź wszystkich oczekujących czytelników
        pthread_cond_broadcast(&m->reading_cond);
        pthread_mutex_unlock(&m->mutex);

        // Pisarz czeka przed kolejnym pisaniem
        usleep(writers_waiting_time);
    }
}

int main(int argc, char *argv[]) {
    
    // Liczba czytelników i pisarzy
    int reader_count = 0;
    int writer_count = 0;

    // Zmienna do przechwytywania opcji z linii poleceń
    int opt;

    // Sprawdzenie, czy podano odpowiednią liczbę argumentów
    if (argc < 5) {
        fprintf(stderr, "Użycie: %s -r <liczba_czytelników> -w <liczba_pisarzy>\nPrzykład: %s -r 5 -w 3\n", argv[0], argv[0]);
        exit(EXIT_FAILURE);
    }

    // Przetwarzanie opcji z linii poleceń
    while ((opt = getopt(argc, argv, "r:w:")) != -1) {
        switch (opt) {
            case 'r':
                reader_count = atoi(optarg);
                break;
            case 'w':
                writer_count = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Użycie: %s -r <liczba_czytelników> -w <liczba_pisarzy>\n\nPrzykład: %s -r 5 -w 3", argv[0], argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Sprawdzenie, czy podano liczbę czytelników i pisarzy
    if (reader_count <= 0 || writer_count <= 0) {
        fprintf(stderr, "Liczba czytelników i pisarzy musi być większa niż 0.\n");
        exit(EXIT_FAILURE);
    }

    // Inicjalizacja monitora
    Monitor Czytelnia;
    monitor_init(&Czytelnia);
    
    // Tablice do przechowywania wątków czytelników i pisarzy
    pthread_t readers[reader_count]; 
    pthread_t writers[writer_count];

    // Tworzenie wątków czytelników i pisarzy
    for (int i = 0; i < reader_count; i++) {
        pthread_create(&readers[i], NULL, reading, (void*)&Czytelnia);
    }
    for (int i = 0; i < writer_count; i++) {
        pthread_create(&writers[i], NULL, writing, (void*)&Czytelnia);
    }

    for (int i = 0; i < reader_count; i++) {
        pthread_join(readers[i], NULL);
    }
    for (int i = 0; i < writer_count; i++) {
        pthread_join(writers[i], NULL);
    }

    return 0;
}