#include <time.h>
#include <stdio.h>
#include <string.h>
#include "core/usb_event.h"

static const char* STATE_NAME[] = {
    "UNPLUGGED","PLUGGED","DENY","CERTIFYING","MOUNTING","MOUNT_FAILED","INSERVICE"
};

static const char* EVENT_NAME[] = {
    "PLUGIN","PLUGOUT","ALLOW_ACCESS","DENY_ACCESS","ALLOW_ACCESS_NONSTR",
    "ALLOW_ACCESS_STR","CERTIFICATE_NOT_VERIFIED","CERTIFICATE_VERIFIED",
    "MOUNT_FAILED","MOUNT_SUCCESS","EXIT"
};

const char* usb_state_name(fsm_state_t s) {
    if (s < 0 || s >= USB_STATE_MAX) return "UNKNOWN_STATE";
    return STATE_NAME[s];
}

const char* usb_event_name(fsm_event_t e) {
    if (e < 0 || e >= USB_EVT_MAX) return "UNKNOWN_EVENT";
    return EVENT_NAME[e];
}

const char* now_hms(void) {
    static char buf[16];
    time_t now = time(NULL);
    struct tm tmv;
    localtime_r(&now, &tmv);
    strftime(buf, sizeof(buf), "%H:%M:%S", &tmv);
    return buf;
}