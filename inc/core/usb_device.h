#ifndef USB_DEVICE_H
#define USB_DEVICE_H

#include <pthread.h>

#define MAX_RULE_LEN   256
#define MAX_KEY_LEN    64
#define MAX_VALUE_LEN  128
#define MAX_PROPERTIES 20

// Forward declaration
typedef struct usb_device_t usb_device_t;

// FSM states
typedef enum {
    USB_STATE_UNPLUGGED = 0,
    USB_STATE_PLUGGED,
    USB_STATE_DENY,
    USB_STATE_CERTIFYING,
    USB_STATE_MOUNTING,
    USB_STATE_MOUNT_FAILED,
    USB_STATE_INSERVICE,
    USB_STATE_MAX
} fsm_state_t;

// Property key-value
typedef struct {
    char key[MAX_KEY_LEN];
    char value[MAX_VALUE_LEN];
} usb_property_t;

// Device object
struct usb_device_t {
    unsigned int id;
    unsigned int event;
    unsigned int target;
    fsm_state_t current_state;
    char device_rule[MAX_RULE_LEN];
    usb_property_t properties[MAX_PROPERTIES];
    int property_count;
    int marked_for_free;       // for safe GC when back to UNPLUGGED
    usb_device_t *next;
};

// Device list ops (thread-safe)
void device_list_add(usb_device_t *dev);
void device_list_mark_remove(usb_device_t *dev);
void device_list_unlink_and_free(usb_device_t *dev);
void device_list_force_cleanup(void);
int  device_list_is_empty(void);
usb_device_t *device_list_find_by_id(unsigned int id);

#endif // USB_DEVICE_H