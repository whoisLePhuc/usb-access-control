#ifndef USBGUARD_INTERFACE_H
#define USBGUARD_INTERFACE_H

#include <dbus/dbus.h>
#include <stdbool.h>
#include "core/usb_device.h"

// Placeholder for real USBGuard DBus integration
//void usbguard_init_connection(void);

// API khởi tạo kết nối DBus
DBusConnection* usbguard_init_connection(void);
int usbguard_storage_device_check(const usb_device_t *info);
void* usb_manager_loop(void *arg);
int apply_device_policy(unsigned int device_id, unsigned int target, bool permanent, unsigned int *out_rule_id);

#endif // USBGUARD_INTERFACE_H