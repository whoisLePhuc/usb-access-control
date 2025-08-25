#ifndef USB_FSM_H
#define USB_FSM_H

#include "core/usb_queue.h"
#include "core/usb_device.h"
#include "core/usb_event.h"

// FSM action function type
typedef void (*fsm_action_t)(usb_device_t *dev);

// FSM transition entry
typedef struct {
    fsm_state_t    new_state;
    fsm_action_t   action;
} fsm_transition_t;

// FSM operations
int fsm_init(void);
void fsm_handle_event(usb_device_t *dev, fsm_event_t ev);

// FSM processing thread
void* fsm_loop(void *arg);

#endif // USB_FSM_H