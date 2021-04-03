#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>


//struct for hold node information
typedef struct node {
    char* value;
    int size;
    struct node *next;
} node;

//struct use to enqueue
typedef struct {
    char* value;
    int size;
} return_data ;

// struct for queue linked-list implementation 
typedef struct {
    node *front;
    node *rear;
    pthread_mutex_t mutex;
} Queue_r;

// Use to initialize queue object
Queue_r* generateQueue()
{

    Queue_r *queue = malloc(sizeof(Queue_r));

    queue->front = NULL;
    queue->rear  = NULL;
    pthread_mutex_init(&queue->mutex, NULL);
    return queue;
}

// enqueue data accept data and data size
void enqueue(Queue_r* queue, char* value, int size)
{
    //char* str ;
    
    node *temp = malloc(sizeof(struct node));
    temp->value = value;
    //strncpy(temp->value, value, sizeof(temp->value));
    temp->size = size;
    temp->next  = NULL;

    pthread_mutex_lock(&queue->mutex);

    if (queue->front == NULL) {

        queue->front = temp;
        queue->rear  = temp;
    }
    else {

        node *old_rear = queue->rear;
        old_rear->next = temp;
        queue->rear    = temp;
    }


    pthread_mutex_unlock(&queue->mutex);
}

// dequeue use to return struct then free the memory
bool dequeue(Queue_r* queue, return_data  *value)
{

    pthread_mutex_lock(&queue->mutex);



    node *front = queue->front;
    if (front == NULL) {

        pthread_mutex_unlock(&queue->mutex);
        return false;
    }

 
    value->value = front->value;
    value->size = front->size;
    queue->front = front->next;
    free(front);


    pthread_mutex_unlock(&queue->mutex);
    return true;
}

void function3(void *queue){

     //sleep(1);
    Queue_r *queue_n = (Queue_r *) queue;
    char * buff = malloc(9);
    buff[5] = 2;
    enqueue(queue_n  , buff, 9);
    enqueue(queue_n  , buff, 9);
    enqueue(queue_n  , buff, 9);
    enqueue(queue_n  , buff, 9);
}

void function2(void *queue){
    Queue_r *queue_n = (Queue_r *) queue;
    for(int i =0; i < 5; i++){
    
    char * buff = malloc(6);
    buff[1] = i;
    enqueue(queue_n  , buff, 6);
    enqueue(queue_n  , buff, 6);
    //sleep(1);
    buff[1] = i;
    enqueue(queue_n  , buff, 6);
    enqueue(queue_n ,buff, 6);
    }

}
void function1(void *queue){
    Queue_r *queue_n  = (Queue_r *) queue;
    return_data qval;
    while (dequeue(queue_n  , &qval)){

        for(int i=0; i < qval.size; i++){
        printf("%hhx" , *(qval.value+i));
        }
        printf("\n");
    //printf("%d;%d\n", qval.value, qval.size);
    }
       
}


int main()
{

    pthread_t id1;
    pthread_t id2;
    Queue_r *my_queue = generateQueue();

    pthread_create(&id1,NULL,function3,  my_queue);
    pthread_create(&id2,NULL,function2,  my_queue);
    pthread_join(id2,NULL);
    pthread_join(id1,NULL);
    
    function1(my_queue);
    printf("\n");

    return 0;
}
