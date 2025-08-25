#ifndef USBGUARD_INTERFACE_H
#define USBGUARD_INTERFACE_H

#include <dbus/dbus.h>
#include "core/usb_device.h"

// Placeholder for real USBGuard DBus integration
//void usbguard_init_connection(void);

// API khởi tạo kết nối DBus
DBusConnection* usbguard_init_connection(void);
int usbguard_storage_device_check(const usb_device_t *info);
void* usb_manager_loop(void *arg);

#endif // USBGUARD_INTERFACE_H