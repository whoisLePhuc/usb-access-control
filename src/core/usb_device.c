#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "core/usb_device.h"
#include "core/usb_event.h"
#include "logger/logger.h"

static usb_device_t *device_list_head = NULL;
static pthread_mutex_t device_list_lock = PTHREAD_MUTEX_INITIALIZER;

void device_list_add(usb_device_t *dev) {
    pthread_mutex_lock(&device_list_lock);
    dev->next = device_list_head;
    device_list_head = dev;
    pthread_mutex_unlock(&device_list_lock);
    logMessage(LOG_INFO, "[Manager]: Added USB device %u", dev->id);
}

void device_list_mark_remove(usb_device_t *dev) {
    pthread_mutex_lock(&device_list_lock);
    dev->marked_for_free = 1;
    pthread_mutex_unlock(&device_list_lock);
    logMessage(LOG_INFO, "[Manager]: Marked device %u for removal", dev->id);
}

void device_list_unlink_and_free(usb_device_t *dev) {
    pthread_mutex_lock(&device_list_lock);
    usb_device_t **pp = &device_list_head;
    while (*pp && *pp != dev) pp = &(*pp)->next;
    if (*pp == dev) {
        *pp = dev->next;
    }
    pthread_mutex_unlock(&device_list_lock);
    logMessage(LOG_INFO, "[GC]: Free device %u", dev->id);
    free(dev);
}

void device_list_force_cleanup(void) {
    pthread_mutex_lock(&device_list_lock);
    usb_device_t *cur = device_list_head;
    device_list_head = NULL;
    pthread_mutex_unlock(&device_list_lock);

    while (cur) {
        usb_device_t *next = cur->next;
        free(cur);
        cur = next;
    }
}

usb_device_t *device_list_find_by_id(unsigned int id) {
    pthread_mutex_lock(&device_list_lock);
    usb_device_t *cur = device_list_head;
    while (cur) {
        if (cur->id == id) {
            pthread_mutex_unlock(&device_list_lock);
            return cur;
        }
        cur = cur->next;
    }
    pthread_mutex_unlock(&device_list_lock);
    return NULL;
}

int device_list_is_empty(void) {
    pthread_mutex_lock(&device_list_lock);
    int empty = (device_list_head == NULL);
    pthread_mutex_unlock(&device_list_lock);
    return empty;
}