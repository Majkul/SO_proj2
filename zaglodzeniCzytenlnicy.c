#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

/*FAWORYZUJACY PISARZY*/

const int reading_time = 1e6;
const int writing_time = 3e6;

const int reading_sleep = 2e6;
const int writing_sleep = 1e6;

struct monitor_t{
    int has_writer; //1 yes; 0 no
    int reader_count;

    int reader_queued;
    int writer_queued;

    pthread_mutex_t mutex;

    pthread_cond_t readers_cond;
    pthread_cond_t writers_cond;

};


/* init */
void monitor_init(struct monitor_t* mtr){
    mtr -> has_writer = 0;
    mtr -> reader_count = 0;
    mtr -> reader_queued = 0;
    mtr -> writer_queued = 0;

    pthread_mutex_init(&mtr -> mutex,NULL);

    pthread_cond_init(&mtr -> readers_cond,NULL);
    pthread_cond_init(&mtr -> writers_cond,NULL);
    
}

void print_status(struct monitor_t* mtr){
    printf("ReaderQ: %d WriterQ: %d [in R: %d W: %d]\n",
        mtr->reader_queued, mtr->writer_queued, mtr->reader_count, mtr->has_writer );
}

void* reader_func(void* arg){
    struct monitor_t* mtx = (struct monitor_t*)arg;
    for(;;){
        pthread_mutex_lock(&mtx->mutex);
        print_status(mtx);
        while(mtx->has_writer || mtx->writer_queued) {
            mtx->reader_queued++;
            //printf("reader queued\n");
            pthread_cond_wait(&mtx->readers_cond,&mtx->mutex);
            //printf("reader dequeued\n");
            mtx->reader_queued--;
        }
        
        //can begin reading
        mtx->reader_count++;
        print_status(mtx);
        pthread_mutex_unlock(&mtx->mutex);
        usleep(reading_time);
        pthread_mutex_lock(&mtx->mutex);

        //finished reading
        mtx->reader_count--;
        
        if(mtx->reader_count <= 0 ){
            //printf("reader,writer\n");
            pthread_cond_signal(&mtx->writers_cond);
        }else if( ! mtx->writer_queued ){
            //printf("reader,reader\n");
            pthread_cond_broadcast(&mtx->readers_cond);
        }
        
        pthread_mutex_unlock(&mtx->mutex);
        //print_status(mtx);
        usleep(reading_sleep);
    }
}
    
void* writer_func(void* arg){
    struct monitor_t* mtx = (struct monitor_t*)arg;
    for(;;){
        
        pthread_mutex_lock(&mtx->mutex);
        print_status(mtx);
        while(mtx->has_writer || mtx->reader_count){
            mtx->writer_queued++;
            //printf("writer queued\n");
            pthread_cond_wait(&mtx->writers_cond,&mtx->mutex);
            //printf("writer dequeued\n");
            mtx->writer_queued--;
        }
        //can write
        mtx->has_writer = 1;
        print_status(mtx);
        pthread_mutex_unlock(&mtx->mutex);
        usleep(writing_time);
        pthread_mutex_lock(&mtx->mutex);

        //finished writing
        mtx->has_writer = 0;
        
        if(mtx->writer_queued){
            //printf("writer,writer\n");
            pthread_cond_signal(&mtx->writers_cond);
        }else{
            //printf("writer,reader\n");
            pthread_cond_broadcast(&mtx->readers_cond);
        }
        pthread_mutex_unlock(&mtx->mutex);
        //print_status(mtx);
        usleep(writing_sleep);
    }

}
        
        
void main(int argc, char* argv[]){
    
    int reader_count_in = 0;
    int writer_count_in = 0;

    ///*
    //Readers
    if(argc < 2){
        printf("Nie zostala podana liczba czytelnikow\n");
        printf("Prosze podac nazwe czytelnikow, po czym pisarzy\n");
        printf("np:   %s 5 2\n",argv[0]);
        return;
    }else{
        reader_count_in = atoi(argv[1]);
    }

    //Writers
    if(argc < 3){
        printf("Nie zostala podana liczba pisarzy\n");
        printf("Prosze podac nazwe czytelnikow, po czym pisarzy\n");
        printf("np:   %s 5 2\n",argv[0]);
        return;
    }else{
        writer_count_in = atoi(argv[2]);
    }
    //*/
    
    struct monitor_t mtr;
    monitor_init(&mtr);
    pthread_t readers[reader_count_in];
    pthread_t writers[writer_count_in];
    for(int i = 0; i < reader_count_in; i++){
        pthread_create(&readers[i],NULL,reader_func,&mtr);
        }
    for(int i = 0; i < writer_count_in; i++){
        pthread_create(&writers[i],NULL,writer_func,&mtr);
    }
    
    for(int i = 0; i < writer_count_in;i++){
        pthread_join(writers[i],NULL);
    }
    for(int i = 0; i < reader_count_in;i++){
        pthread_join(readers[i],NULL);
    }    


}

