#ifndef USB_QUEUE_H
#define USB_QUEUE_H

#include <pthread.h>
#include "usb_event.h"

#define EVENT_QUEUE_SIZE 32

// Event queue entry
typedef struct {
    usb_device_t *dev;
    fsm_event_t ev;
} fsm_event_entry_t;

// Thread-safe event queue
typedef struct {
    fsm_event_entry_t buffer[EVENT_QUEUE_SIZE];
    int head, tail, count;
    pthread_mutex_t lock;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
} event_queue_t;

// Queue operations
int queue_init(event_queue_t *q);
void queue_destroy(event_queue_t *q);
int enqueue_event(event_queue_t *q, usb_device_t *dev, fsm_event_t ev);
int dequeue_event(event_queue_t *q, fsm_event_entry_t *out);

// Global event queue
extern event_queue_t g_event_queue;

#endif // USB_QUEUE_H