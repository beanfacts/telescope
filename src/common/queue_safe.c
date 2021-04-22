#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

#include "queue_safe.h"

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
void enqueue(Queue_r* queue, void* value, int size)
{
    node *temp = malloc(sizeof(struct node));
    temp->value = value;
    temp->size = size;
    temp->next  = NULL;
    pthread_mutex_lock(&queue->mutex);

    if (queue->front == NULL)
    {
        queue->front = temp;
        queue->rear  = temp;
    }
    else
    {
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
    if (front == NULL)
    {
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