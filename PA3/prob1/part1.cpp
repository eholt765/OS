/*

Author: Elijah Holt
Date: 3/12/2022
CSCE 451 PA3

*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

void* producer(void* prod);
void* consumer(void* cons);

sem_t mutex;
sem_t empty;
sem_t full;

char* buffer;

volatile int numb_insert_items;

volatile int index = 0;

int main(int argc, char* argv[]) {
    
    int buff_length = atoi(argv[2]); //-b buffer of length X bytes
    int numb_producers = atoi(argv[4]); //-p number of producer threads
    int numb_consumers = atoi(argv[6]); //-c number of consumer threads
    numb_insert_items = atoi(argv[8]); //number of inster items

    // create buffer & initialise semaphores
    buffer = (char*)malloc(sizeof(char) * buff_length);
    sem_init(&mutex, 0, 1);
    sem_init(&full, 0, 0);
    sem_init(&empty, 0, buff_length);

    // create -p # of producer threads
    pthread_t p_thread;
    int* producer_list = new int[numb_producers];

    for (int i = 0; i < numb_producers; i++) {
        producer_list[i] = i+1;
    }

    
    for (int i = 0; i < numb_producers; i++) {
        if (pthread_create(&p_thread, NULL, producer, &producer_list[i])) {
            fprintf(stderr, "Producer thread %d failed\n", producer_list[i]);
            return -1;
        }
    }

    // create -c # of consumer threads
    pthread_t c_thread;
    int* consumer_list = new int[numb_consumers];

    for (int i = 0; i < numb_consumers; i++) {
        consumer_list[i] = i + 1;
    }

    
    for (int i = 0; i < numb_consumers; i++) {
        if (pthread_create(&c_thread, NULL, consumer, &consumer_list[i])) {
            fprintf(stderr, "Consumer thread %d failed\n", consumer_list[i]);
            return -1;
        }
    }

    //clean up producers
    for (int i = 0; i < numb_producers; i++) {
        pthread_join(p_thread, NULL);
    }

    //clean up consumers
    for (int i = 0; i < numb_consumers; i++) {
        pthread_join(c_thread, NULL);
    }

    return 0;
}

void* producer(void* prod) {
    int* threadid = (int*)prod;

    while (1) {
        sem_wait(&empty);
        sem_wait(&mutex);

        // insert X into the first available slot in the buffer
        if (numb_insert_items > 0) {
            char item = 'X';
            buffer[index] = item;
            printf("p:<%u>, item: %c, at %d\n", *threadid, item, index);
            index++;
            numb_insert_items--;
        }
        else {
            sem_post(&mutex);
            sem_post(&full);

            return 0;
        }

        sem_post(&mutex);
        sem_post(&full);
    }
}

void* consumer(void* cons) {
    int* threadid = (int*)cons;

    while (1) {
        sem_wait(&full);
        sem_wait(&mutex);

        // remove X from the last used slot in the buffer
        if (index > 0) {
            index--;
            char item = buffer[index];
            buffer[index] = '\0';
            printf("c:<%u>, item: %c, at %d\n", *threadid, item, index);
        }
        else {
            sem_post(&mutex);
            sem_post(&empty);

            return 0;
        }

        sem_post(&mutex);
        sem_post(&empty);
    }
    pthread_exit(NULL);
}