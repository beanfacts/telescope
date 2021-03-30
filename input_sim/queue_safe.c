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
Queue_r* qcreate()
{
    // crea la coda
    Queue_r *queue = malloc(sizeof(Queue_r));

    // inizializza la coda
    queue->front = NULL;
    queue->rear  = NULL;
    pthread_mutex_init(&queue->mutex, NULL);
    return queue;
}

// enqueue data accept data and data size
void enqueue(Queue_r* queue, char* value, int size)
{

    node *temp = malloc(sizeof(struct node));
    temp->value = value;
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

void function3(void *bright){

     sleep(1);
    Queue_r *data = (Queue_r *) bright;
    enqueue(data , 110, 4);
    enqueue(data , 220, 4);
    enqueue(data , 330, 4);
    enqueue(data , 440, 4);
}

void function2(void *bright){

    Queue_r *data  = (Queue_r *) bright;
    enqueue(data , 10, 4);
    enqueue(data , 20, 4);
    enqueue(data , 30, 4);
    enqueue(data , 40, 4);
}
void function1(void *bright){
    Queue_r *data  = (Queue_r *) bright;
    return_data qval;
    while (dequeue(data , &qval)){

         printf("%d;%d\n", qval.value, qval.size);
    }
       
}

//for demonstration
int main()
{

    pthread_t id1;
    pthread_t id2;
    Queue_r *my_queue = qcreate();


    printf("la coda my_queue contiene: ");
    pthread_create(&id1,NULL,function3,  my_queue);
    pthread_create(&id2,NULL,function2,  my_queue);
    pthread_join(id2,NULL);
    pthread_join(id1,NULL);
    
    function1(my_queue);
    printf("\n");

    return 0;
}
