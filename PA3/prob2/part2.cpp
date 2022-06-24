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

typedef struct {
	sem_t mutex;
	sem_t next;
	int thread_count;
} mon;

mon monit;

typedef struct {
	// condition variable fields
	int count;
	sem_t semaphore;
} cond;

int cond_count(cond* cv) {
	return cv->count;
}

void wait(cond* cv) {
	// give up exclusive access to monitor
	// and suspend appropriate thread
	// implement either Hoare or Mesa paradigm
	cv->count++;
	if (monit.thread_count > 0) {
		sem_post(&monit.next);
	}
	else {
		sem_post(&monit.mutex);
	}
	sem_wait(&cv->semaphore);
	cv->count--;
}

void signal(cond* cv) {
	// unblock suspended thread at head of queue
	// implement either Hoare or Mesa paradigm
	if (cond_count(cv) > 0) {
		monit.thread_count++;
		sem_post(&cv->semaphore);
		sem_wait(&monit.next);
		monit.thread_count--;
	}
}



void* producer(void* prod);
void* consumer(void* cons);
char generate_random_alphabet();
void mon_init();
void mon_insert(char alpha, int id);
void mon_remove(int id);



cond empty;
cond full;


char* buffer;

volatile int numb_insert_items;
volatile int buff_length;

volatile int in = 0;
volatile int out = 0;
volatile int curr_items_counter = 0;
volatile int all_items_counter = 0;



int main(int argc, char* argv[]) {

	buff_length = atoi(argv[2]); //-b buffer of length X bytes
	int numb_producers = atoi(argv[4]); //-p number of producer threads
	int numb_consumers = atoi(argv[6]); //-c number of consumer threads
	numb_insert_items = atoi(argv[8]); //number of inster items

	// create buffer & initialise semaphores
	buffer = (char*)malloc(sizeof(char) * buff_length);
	mon_init();

	// create -p # of producer threads
	pthread_t p_thread;
	int* producer_list = new int[numb_producers];

	for (int i = 0; i < numb_producers; i++) {
		producer_list[i] = i + 1;
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
	char alpha;

	while (1) {
		alpha = generate_random_alphabet();
		mon_insert(alpha, *threadid);
	}

	return 0;
}

void* consumer(void* cons) {
	int* threadid = (int*)cons;

	while (1) {
		mon_remove(*threadid);
	}

	return 0;
}


char generate_random_alphabet() {
	char randomletter = 'A' + rand() % 26;
	return randomletter;
}


void mon_init() {
	monit.thread_count = 0;
	sem_init(&monit.next, 0, 0);
	sem_init(&monit.mutex, 0, 1);
	sem_init(&full.semaphore, 0, 0);
	sem_init(&empty.semaphore, 0, 0);

}

void mon_insert(char alpha, int id) {
	sem_wait(&monit.mutex);

	if (all_items_counter >= numb_insert_items) {
		sem_post(&monit.mutex);
		pthread_exit(0);
	}


	while (curr_items_counter == buff_length) {
		wait(&full);

	}
	if (all_items_counter < numb_insert_items && curr_items_counter < buff_length) {

		char item = alpha;
		buffer[in] = item;
		printf("p:<%u>, item: %c, at %d\n", id, item, in);
		in = (in + 1) % buff_length;
		curr_items_counter++;
		all_items_counter++;
	}
	signal(&empty);
	if (monit.thread_count > 0) {
		sem_post(&monit.next);
	}
	else {
		sem_post(&monit.mutex);
	}

}

void mon_remove(int id) {
	char result;
	sem_wait(&monit.mutex);

	if (all_items_counter >= numb_insert_items && curr_items_counter == 0) {
		exit(0);
	}

	while (curr_items_counter == 0) {
		wait(&empty);
	}

	if (all_items_counter <= numb_insert_items && curr_items_counter > 0) {
		result = buffer[out];
		printf("c:<%u>, item: %c, at %d\n", id, result, out);
		out = (out + 1) % buff_length;
		curr_items_counter--;
	}

	signal(&full);

	if (monit.thread_count > 0) {
		sem_post(&monit.next);
	}
	else {
		sem_post(&monit.mutex);
	}


}