#include <stdio.h>
#include "core/usb_queue.h"

int queue_init(event_queue_t *q) {
    if (!q) return -1; // NULL pointer
    q->head = q->tail = q->count = 0;
    if (pthread_mutex_init(&q->lock, NULL) != 0) {
        fprintf(stderr, "Failed to initialize queue mutex\n");
        return -2;
    }
    if (pthread_cond_init(&q->not_full, NULL) != 0) {
        fprintf(stderr, "Failed to initialize queue condition variable (not_full)\n");
        pthread_mutex_destroy(&q->lock);
        return -3;
    }
    if (pthread_cond_init(&q->not_empty, NULL) != 0) {
        fprintf(stderr, "Failed to initialize queue condition variable (not_empty)\n");
        pthread_mutex_destroy(&q->lock);
        pthread_cond_destroy(&q->not_full);
        return -4;
    }    
    return 0;
}

void queue_destroy(event_queue_t *q) {
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->not_full);
    pthread_cond_destroy(&q->not_empty);
}

int enqueue_event(event_queue_t *q, usb_device_t *dev, fsm_event_t ev) {
    pthread_mutex_lock(&q->lock);
    while (q->count == EVENT_QUEUE_SIZE) {
        pthread_cond_wait(&q->not_full, &q->lock);
    }
    q->buffer[q->tail].dev = dev;
    q->buffer[q->tail].ev  = ev;
    q->tail = (q->tail + 1) % EVENT_QUEUE_SIZE;
    q->count++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
    return 0;
}

int dequeue_event(event_queue_t *q, fsm_event_entry_t *out) {
    pthread_mutex_lock(&q->lock);
    while (q->count == 0) {
        pthread_cond_wait(&q->not_empty, &q->lock);
    }
    *out = q->buffer[q->head];
    q->head = (q->head + 1) % EVENT_QUEUE_SIZE;
    q->count--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->lock);
    return 0;
}