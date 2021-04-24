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

Queue_r* generateQueue();

void enqueue(Queue_r *queue, void *value, int size);

bool dequeue(Queue_r *queue, return_data *value);