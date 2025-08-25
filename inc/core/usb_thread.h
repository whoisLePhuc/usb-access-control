#ifndef USB_THREAD_H
#define USB_THREAD_H

#include "usb_queue.h"

// Simulated USB manager thread (generates events for 3 devices)
void* usb_manager_thread(void *arg);

#endif // USB_THREAD_H