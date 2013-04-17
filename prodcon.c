#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h> 
#include <semaphore.h>

// constants
#define BUFFER_SIZE 5
#define TRUE 1
#define FALSE 0
#define MAX_SLEEP 20 // upper bound on random sleeping

typedef int buffer_item;


// shared buffer
buffer_item buffer[BUFFER_SIZE];

// shared variables associated with buffer
int in = 0, out = 0, totalProduced = 0;
int totalConsumed = 0, bufferElements = 0;

// buffer operations
int insert_item(buffer_item item){
  // check if space in buffer
  if(bufferElements < BUFFER_SIZE){
	 // insert item into buffer
	 buffer[in] = item;
	 in = (in + 1)%BUFFER_SIZE;
	 bufferElements++;
	 return 0;
	 
  } else {
	 // buffer was already full
	 return -1;
  }
}

int remove_item(buffer_item *item){
  // check if buffer has items
  if(bufferElements > 0){
	 *item = buffer[out];
	 out = (out + 1)%BUFFER_SIZE;
	 bufferElements--;
	 return 0;
	 
  } else {
	 // buffer was empty
	 return -1;
  }
}


// special struct to pass paramters to pthreads
typedef struct {
  int id;
  sem_t * mut;
  sem_t * full;
  sem_t * empty;
} threadParams;

// producer thread worker
void * producer(void * param){
  buffer_item item;
  threadParams * producerParams =(threadParams *) param;
  int id = producerParams->id;
  
  while(TRUE){
  /*sleep for a random period of time*/
	 sleep(rand() % MAX_SLEEP);
	 // insert random item
	 item = rand();
	 
	 // acquire empty and mutex
	 sem_wait(producerParams->empty);
	 sem_wait(producerParams->mut);
	 
	 // execute critical section
	 if(insert_item(item)){
		fprintf(stderr, "producer reports error");
	 } else{
		printf("Producer %d produced %d\n",id,item);
	 }
	 totalProduced++;
	 
	 // signal mutex and full
	 sem_post(producerParams->mut);
	 sem_post(producerParams->full);
  }
}

// consumer thread worker
void * consumer(void * param){
  buffer_item item;
  threadParams * consumerParams =(threadParams *) param;
  int id = consumerParams->id;
  
  while(TRUE){
	 /*sleep for a random period of time*/
	 sleep(rand() % MAX_SLEEP);
	 
	 // acquire empty and mutex
	 sem_wait(consumerParams->full);
	 sem_wait(consumerParams->mut);
	 
	 // execute critical section
	 if(remove_item(&item)){
		fprintf(stderr, "consumer reports error \n");
	 } else {
		printf("\tConsumer %d consumed %d\n",id,item);
	 }
	 totalConsumed++;
	 
	 // signal mutex and full
	 sem_post(consumerParams->mut);
	 sem_post(consumerParams->empty);
  }
}


int main(int argc, char * argv[]){
  // variables
  sem_t full, empty, mutex;
  int numConsumers, numProducers, sleepTime;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  int counter;
  
  // sanity check command line arguments
  if(argc < 4){
    fprintf(stderr, "enter number of producers/ consumers and sleep time \n \n");
    return -1;
  }

  // get number of producers and consumers and sleep time
  sleepTime = atoi(argv[1]);
  numProducers = atoi(argv[2]);
  numConsumers = atoi(argv[3]);

  // initialization
  sem_init(&full, 0, 0);
  sem_init(&empty, 0, 5);
  sem_init(&mutex, 0, 1);
  
  // create producers
  pthread_t producers[numProducers];
  threadParams prodParams[numProducers];

  for(counter = 0; counter < numProducers; counter++){
	 // set parameters
	 prodParams[counter].mut = &mutex;
	 prodParams[counter].empty = &empty;
    prodParams[counter].full = &full;
	 prodParams[counter].id = counter;
	 
	 // make thread
	 pthread_create(&producers[counter], &attr, producer, &prodParams[counter]);
  }
  
  // create consumers
  pthread_t consumers[numConsumers];
  threadParams conParams[numConsumers];
  for(counter = 0; counter < numConsumers; counter++){
	 
	 // set parameters
	 conParams[counter].mut = &mutex;
	 conParams[counter].empty = &empty;
    conParams[counter].full = &full;
	 conParams[counter].id = counter;
	 
	 // make thread
	 pthread_create(&consumers[counter], &attr, consumer, &conParams[counter]);
  }

  // sleep
  sleep(sleepTime);
  
  // acquire mutex
  sem_wait(&mutex);
  
  // print totals
  printf("Items produced: %d \n", totalProduced);
  printf("Items consumed: %d \n \n", totalConsumed);
  
  // exit
  exit(0);
}
