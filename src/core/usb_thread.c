#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "core/usb_thread.h"
#include "core/usb_event.h"
#include "core/usb_device.h"
#include "core/usb_queue.h"

extern event_queue_t g_event_queue;

void* usb_manager_thread(void *arg) {
    (void)arg;
    unsigned int device_id_counter = 1;

    sleep(1);
    usb_device_t *dev1 = (usb_device_t*)calloc(1, sizeof(*dev1));
    dev1->id = device_id_counter++;
    dev1->current_state = USB_STATE_UNPLUGGED;
    dev1->property_count = 1;
    strncpy(dev1->properties[0].key, "type", MAX_KEY_LEN-1);
    strncpy(dev1->properties[0].value, "storage_device", MAX_VALUE_LEN-1);
    device_list_add(dev1);
    enqueue_event(&g_event_queue, dev1, USB_EVT_PLUGIN);

    sleep(3);
    usb_device_t *dev2 = (usb_device_t*)calloc(1, sizeof(*dev2));
    dev2->id = device_id_counter++;
    dev2->current_state = USB_STATE_UNPLUGGED;
    dev2->property_count = 1;
    strncpy(dev2->properties[0].key, "type", MAX_KEY_LEN-1);
    strncpy(dev2->properties[0].value, "non_storage_device", MAX_VALUE_LEN-1);
    device_list_add(dev2);
    enqueue_event(&g_event_queue, dev2, USB_EVT_PLUGIN);

    sleep(3);
    usb_device_t *dev3 = (usb_device_t*)calloc(1, sizeof(*dev3));
    dev3->id = device_id_counter++;
    dev3->current_state = USB_STATE_UNPLUGGED;
    dev3->property_count = 1;
    strncpy(dev3->properties[0].key, "type", MAX_KEY_LEN-1);
    strncpy(dev3->properties[0].value, "unrecognized", MAX_VALUE_LEN-1);
    device_list_add(dev3);
    enqueue_event(&g_event_queue, dev3, USB_EVT_PLUGIN);

    // unplug sequence
    sleep(5);
    enqueue_event(&g_event_queue, dev1, USB_EVT_PLUGOUT);
    device_list_mark_remove(dev1);

    sleep(2);
    enqueue_event(&g_event_queue, dev2, USB_EVT_PLUGOUT);
    device_list_mark_remove(dev2);

    sleep(2);
    enqueue_event(&g_event_queue, dev3, USB_EVT_PLUGOUT);
    device_list_mark_remove(dev3);

    return NULL;
}