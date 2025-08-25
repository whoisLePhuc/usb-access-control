#ifndef USB_EVENT_H
#define USB_EVENT_H

#include "usb_device.h"

// Fsm events
typedef enum {
    USB_EVT_PLUGIN = 0,
    USB_EVT_PLUGOUT,
    USB_EVT_ALLOW_ACCESS,
    USB_EVT_DENY_ACCESS,
    USB_EVT_ALLOW_ACCESS_NONSTR,
    USB_EVT_ALLOW_ACCESS_STR,
    USB_EVT_CERTIFICATE_NOT_VERIFIED,
    USB_EVT_CERTIFICATE_VERIFIED,
    USB_EVT_MOUNT_FAILED,
    USB_EVT_MOUNT_SUCCESS,
    USB_EVT_EXIT,
    USB_EVT_MAX
} fsm_event_t;

// stringify helpers
const char* usb_state_name(fsm_state_t s);  // state to string
const char* usb_event_name(fsm_event_t e); // event to string
const char* now_hms(void); // current time as HH:MM:SS

#endif // USB_EVENT_H