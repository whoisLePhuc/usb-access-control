#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include "fsm/usb_fsm.h"
#include "core/usb_event.h"
#include "utils/utils.h"
#include "interface/usbguard_interface.h"
#include "config.h"
#include "security/validation.h"
#include "logger/logger.h" 

static fsm_transition_t fsm[USB_STATE_MAX][USB_EVT_MAX];

static void action_plugin(usb_device_t *device) {
    logMessage(LOG_INFO, "[PLUGIN]: USB %u state %s", device->id, usb_state_name(device->current_state));
    if (usbguard_storage_device_check(device)) {
        enqueue_event(&g_event_queue, device, USB_EVT_ALLOW_ACCESS_STR);
    } else {
        enqueue_event(&g_event_queue, device, USB_EVT_ALLOW_ACCESS_NONSTR);
    }
}

static void action_plugout(usb_device_t *device) {
    logMessage(LOG_INFO, "[PLUGOUT]: USB %u state %s", device->id, usb_state_name(device->current_state));
    device_list_mark_remove(device);
}

static void action_deny_access(usb_device_t *device) {
    unsigned int rule_id = 0;
    int ret = apply_device_policy(device->id, 0, false, &rule_id); // deny device, non-permanent
    if (ret == 0) {
        logMessage(LOG_INFO, "[DENY]: USB %u state %s", device->id, usb_state_name(device->current_state));
    } else {
        logMessage(LOG_WARNING, "[DENY]: USB %u state %s error code %d", device->id, usb_state_name(device->current_state), ret);
    }
    logMessage(LOG_INFO, "[DENY]: Replug USB %u to reconnect", device->id);
    return ret;
}

static void action_allow_access_str(usb_device_t *device) {
    if (!device) {
        fprintf(stderr, "Error: NULL device in action_allow_access_str\n");
        return;
    }
    // Validate device properties
    if (validate_device_properties(device->properties, device->property_count, MAX_PROPERTIES) != 0) {
        fprintf(stderr, "Error: Invalid device properties for device %u, error %d\n", device->id, validate_device_properties(device->properties, device->property_count, MAX_PROPERTIES));
        enqueue_event(&g_event_queue, device, USB_EVT_CERTIFICATE_NOT_VERIFIED);
        return;
    }
    logMessage(LOG_INFO, "[ALLOW ACCESS STR]: USB %u state %s",
           device->id, usb_state_name(device->current_state));
    // Get configuration
    const app_config_t *config = config_get();
    if (!config) {
        logMessage(LOG_ERROR, "Configuration not available\n");
        enqueue_event(&g_event_queue, device, USB_EVT_CERTIFICATE_NOT_VERIFIED);
        return;
    }
    // Lấy VID/PID từ property "id"
    const char* id_string_value = NULL;
    char vendor_id[MAX_KEY_LEN] = {0};
    char product_id[MAX_KEY_LEN] = {0};
    for (int i = 0; i < device->property_count; ++i) {
        if (strcmp(device->properties[i].key, "id") == 0) {
            id_string_value = device->properties[i].value;
            break;
        }
    }
    if (!id_string_value) {
        logMessage(LOG_WARNING, "ID property not found");
        enqueue_event(&g_event_queue, device, USB_EVT_CERTIFICATE_NOT_VERIFIED);
        return;
    }
    // Validate ID string
    if (validate_string(id_string_value, MAX_VALUE_LEN) != 0) {
        fprintf(stderr, "Error: Invalid ID string for device %u\n", device->id);
        enqueue_event(&g_event_queue, device, USB_EVT_CERTIFICATE_NOT_VERIFIED);
        return;
    }
    if (parse_id_string(id_string_value, vendor_id, product_id) != 0) {
        logMessage(LOG_ERROR, "Failed to parse VID/PID from id string: %s", id_string_value);
        enqueue_event(&g_event_queue, device, USB_EVT_CERTIFICATE_NOT_VERIFIED);
        return;
    }
    logMessage(LOG_INFO, "Vendor ID: %s, Product ID: %s", vendor_id, product_id);
    // Tìm phân vùng USB_SIG với retry logic
    char *sig_path = NULL;
    int retry_count = config->usb_sig_retry_count;
    int retry_delay_ms = config->usb_sig_retry_delay_ms;
    logMessage(LOG_INFO, "Searching for USB_SIG partition (retry count: %d, delay: %dms)...", retry_count, retry_delay_ms);
    for (int attempt = 1; attempt <= retry_count; attempt++) {
        logMessage(LOG_DEBUG, "Attempt %d/%d: Searching for USB_SIG partition...", attempt, retry_count);
        sig_path = find_usb_sig_partition(vendor_id, product_id);
        if (sig_path) {
            logMessage(LOG_INFO, "USB_SIG partition found on attempt %d at: %s", attempt, sig_path);
            break;
        }
        if (attempt < retry_count) {
            logMessage(LOG_DEBUG, "USB_SIG partition not found on attempt %d, waiting %dms before retry...", attempt, retry_delay_ms);
            
            // Convert milliseconds to nanoseconds for nanosleep
            struct timespec delay;
            delay.tv_sec = retry_delay_ms / 1000;
            delay.tv_nsec = (retry_delay_ms % 1000) * 1000000;
            nanosleep(&delay, NULL);
        }
    }
    if (!sig_path) {
        logMessage(LOG_WARNING, "USB_SIG partition not found after %d attempts. Device may not have signature partition or mount is too slow.", retry_count);
        enqueue_event(&g_event_queue, device, USB_EVT_CERTIFICATE_NOT_VERIFIED);
        return;
    }
    // Validate path
    if (validate_file_path(sig_path) != 0) {
        fprintf(stderr, "Error: Invalid signature partition path: %s, error %d\n", sig_path, validate_file_path(sig_path));
        free(sig_path);
        enqueue_event(&g_event_queue, device, USB_EVT_CERTIFICATE_NOT_VERIFIED);
        return;
    }
    logMessage(LOG_INFO, "USB_SIG partition validated successfully at: %s", sig_path);
    // Đọc chứng chỉ trực tiếp từ phân vùng
    size_t cert_len = 0;
    unsigned char *cert_buf = read_usb_cert(sig_path, &cert_len);
    free(sig_path);
    
    if (!cert_buf) {
        logMessage(LOG_ERROR, "Failed to read certificate from USB");
        enqueue_event(&g_event_queue, device, USB_EVT_CERTIFICATE_NOT_VERIFIED);
        return;
    }
    // Validate certificate data
    if (validate_certificate_data(cert_buf, cert_len) != 0) {
        fprintf(stderr, "Error: Invalid certificate data for device %u\n", device->id);
        free(cert_buf);
        enqueue_event(&g_event_queue, device, USB_EVT_CERTIFICATE_NOT_VERIFIED);
        return;
    }
    // Verify chứng chỉ USB với CA
    int ok = verify_usb_cert(cert_buf, cert_len, config->ca_cert_path);
    free(cert_buf);
    if (ok == 1) {
        logMessage(LOG_INFO, "USB certificate verified successfully");
        enqueue_event(&g_event_queue, device, USB_EVT_CERTIFICATE_VERIFIED);
    } else {
        logMessage(LOG_ERROR, "USB certificate verification FAILED");
        enqueue_event(&g_event_queue, device, USB_EVT_CERTIFICATE_NOT_VERIFIED);
    }
}

static void action_allow_access_non_str(usb_device_t *device) {
    logMessage(LOG_INFO, "[ALLOW ACCESS NON STR]: USB %u state %s", device->id, usb_state_name(device->current_state));
    enqueue_event(&g_event_queue, device, USB_EVT_MOUNT_SUCCESS);
}

static void action_certificate_not_verified(usb_device_t *device) {
    logMessage(LOG_WARNING, "[NOT VERIFY]: USB %u state %s", device->id, usb_state_name(device->current_state));
}

static void action_certificate_verified(usb_device_t *device) {
    logMessage(LOG_INFO, "[VERIFY]: USB %u state %s", device->id, usb_state_name(device->current_state));
}

static void action_mount_failed(usb_device_t *device) {
    logMessage(LOG_ERROR, "[MOUNT FAILED]: USB %u state %s", device->id, usb_state_name(device->current_state));
}

static void action_mount_success(usb_device_t *device) {
    logMessage(LOG_INFO, "[MOUNT SUCCESS]: USB %u state %s", device->id, usb_state_name(device->current_state));
}

int fsm_init(void) {
    // Validate state and event ranges
    if (USB_STATE_MAX <= 0 || USB_EVT_MAX <= 0) {
        fprintf(stderr, "Error: Invalid state or event count\n");
        return -1;
    }
    // Initialize FSM table with safe defaults
    for (int s = 0; s < USB_STATE_MAX; s++) {
        for (int e = 0; e < USB_EVT_MAX; e++) {
            fsm[s][e].new_state = (fsm_state_t)s;
            fsm[s][e].action = NULL;
        }
    }
    // Set up state transitions
    fsm[USB_STATE_UNPLUGGED][USB_EVT_PLUGIN] = (fsm_transition_t){USB_STATE_PLUGGED, action_plugin};
    fsm[USB_STATE_PLUGGED][USB_EVT_PLUGOUT] = (fsm_transition_t){USB_STATE_UNPLUGGED, action_plugout};
    fsm[USB_STATE_PLUGGED][USB_EVT_DENY_ACCESS] = (fsm_transition_t){USB_STATE_DENY, action_deny_access};
    fsm[USB_STATE_PLUGGED][USB_EVT_ALLOW_ACCESS_STR] = (fsm_transition_t){USB_STATE_CERTIFYING, action_allow_access_str};
    fsm[USB_STATE_PLUGGED][USB_EVT_ALLOW_ACCESS_NONSTR] = (fsm_transition_t){USB_STATE_MOUNTING, action_allow_access_non_str};
    fsm[USB_STATE_DENY][USB_EVT_PLUGOUT] = (fsm_transition_t){USB_STATE_UNPLUGGED, action_plugout};
    fsm[USB_STATE_CERTIFYING][USB_EVT_PLUGOUT] = (fsm_transition_t){USB_STATE_UNPLUGGED, action_plugout};
    fsm[USB_STATE_CERTIFYING][USB_EVT_CERTIFICATE_NOT_VERIFIED] = (fsm_transition_t){USB_STATE_DENY, action_certificate_not_verified};
    fsm[USB_STATE_CERTIFYING][USB_EVT_CERTIFICATE_VERIFIED] = (fsm_transition_t){USB_STATE_MOUNTING, action_certificate_verified};
    fsm[USB_STATE_MOUNTING][USB_EVT_PLUGOUT] = (fsm_transition_t){USB_STATE_UNPLUGGED, action_plugout};
    fsm[USB_STATE_MOUNTING][USB_EVT_MOUNT_FAILED] = (fsm_transition_t){USB_STATE_MOUNT_FAILED, action_mount_failed};
    fsm[USB_STATE_MOUNTING][USB_EVT_MOUNT_SUCCESS] = (fsm_transition_t){USB_STATE_INSERVICE, action_mount_success};
    fsm[USB_STATE_MOUNT_FAILED][USB_EVT_PLUGOUT] = (fsm_transition_t){USB_STATE_UNPLUGGED, action_plugout};
    fsm[USB_STATE_MOUNT_FAILED][USB_EVT_MOUNT_SUCCESS] = (fsm_transition_t){USB_STATE_INSERVICE, action_mount_success};
    fsm[USB_STATE_INSERVICE][USB_EVT_PLUGOUT] = (fsm_transition_t){USB_STATE_UNPLUGGED, action_plugout};
    logMessage(LOG_INFO, "FSM initialized successfully with %d states and %d events", USB_STATE_MAX, USB_EVT_MAX);
    return 0;
}

static void fsm_maybe_gc(usb_device_t *dev) {
    if (!dev) return;
    if (dev->marked_for_free && dev->current_state == USB_STATE_UNPLUGGED) {
        device_list_unlink_and_free(dev);
    }
}

void fsm_handle_event(usb_device_t *dev, fsm_event_t ev) {
    if (!dev) return;
    if (dev->current_state >= USB_STATE_MAX || ev >= USB_EVT_MAX) {
        logMessage(LOG_ERROR, "Invalid state/event! State=%d, Event=%d", dev->current_state, ev);
        return;
    }
    fsm_transition_t *t = &fsm[dev->current_state][ev];
    if (t->action) {
        t->action(dev);
    } else {
        logMessage(LOG_WARNING, "[No Action]: USB %u in state %s receives %s. State unchanged.", dev->id, usb_state_name(dev->current_state), usb_event_name(ev));
    }
    fsm_state_t old = dev->current_state;
    dev->current_state = t->new_state;
    logMessage(LOG_DEBUG, "-> New state: %s (was %s)",
           usb_state_name(dev->current_state), usb_state_name(old));
    fsm_maybe_gc(dev);
}

// FSM thread: waits on queue and dispatches
void* fsm_loop(void *arg) {
    event_queue_t *q = (event_queue_t *)arg;
    fsm_event_entry_t ev;
    for (;;) {
        dequeue_event(q, &ev);
        if (ev.ev == USB_EVT_EXIT) {
            logMessage(LOG_INFO, "FSM thread received EXIT event. Exiting...");
            break;
        }
        fsm_handle_event(ev.dev, ev.ev);
    }
    return NULL;
}