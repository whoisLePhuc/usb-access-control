#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libudev.h>
#include <mntent.h>
#include <dbus/dbus.h>
#include <stdbool.h>
#include "interface/usbguard_interface.h"
#include "core/usb_queue.h"
#include "core/usb_event.h"

// External event queue to push USB events
extern event_queue_t g_event_queue;

// Helper to safely copy strings with size limit
static inline void safe_strcpy(char *dst, size_t dst_sz, const char *src) {
  if (!dst || dst_sz == 0) return;
  if (!src) { dst[0] = '\0'; return; }
  strncpy(dst, src, dst_sz - 1);
  dst[dst_sz - 1] = '\0';
}

// Log DBus error if set
static void log_dbus_error(const char *where, DBusError *err) {
  if (dbus_error_is_set(err)) {
    fprintf(stderr, "[DBus] %s: %s\n", where, err->message);
    dbus_error_free(err);
  }
}

// Initialize DBus connection and set up match rule
DBusConnection* usbguard_init_connection(void) {
  DBusError err;
  dbus_error_init(&err);
  DBusConnection *conn = dbus_bus_get_private(DBUS_BUS_SYSTEM, &err);
  if (!conn) {
    log_dbus_error("dbus_bus_get_private", &err);
    fprintf(stderr, "Cannot connect to system bus.\n");
    return NULL;
  }
  dbus_connection_set_exit_on_disconnect(conn, FALSE);
  const char *rule = "type='signal',interface='org.usbguard.Devices1',member='DevicePresenceChanged'";
  dbus_bus_add_match(conn, rule, &err);
  log_dbus_error("dbus_bus_add_match", &err);
  dbus_connection_flush(conn);
  return conn;
}

// Listen for USB device events and parse into usb_device_t structure
static usb_device_t *usbguard_listen_event(DBusConnection *conn) {
  if (!conn) return NULL;
  dbus_connection_read_write(conn, 0);
  DBusMessage *msg = dbus_connection_pop_message(conn);
  if (msg == NULL) {
    return NULL;
  }
  if (!dbus_message_is_signal(msg, "org.usbguard.Devices1", "DevicePresenceChanged")) {
    dbus_message_unref(msg);
    return NULL;
  }
  DBusMessageIter iter;
  if (!dbus_message_iter_init(msg, &iter)) {
    dbus_message_unref(msg);
    return NULL;
  }
  int t0 = dbus_message_iter_get_arg_type(&iter);
  DBusMessageIter it = iter;
  dbus_message_iter_next(&it);
  int t1 = dbus_message_iter_get_arg_type(&it);
  dbus_message_iter_next(&it);
  int t2 = dbus_message_iter_get_arg_type(&it);
  dbus_message_iter_next(&it);
  int t3 = dbus_message_iter_get_arg_type(&it);
  dbus_message_iter_next(&it);
  int t4 = dbus_message_iter_get_arg_type(&it);
  if (t0 != DBUS_TYPE_UINT32 || t1 != DBUS_TYPE_UINT32 || t2 != DBUS_TYPE_UINT32 ||
      t3 != DBUS_TYPE_STRING  || t4 != DBUS_TYPE_ARRAY) {
    dbus_message_unref(msg);
    return NULL;
  }
  usb_device_t *newdev = (usb_device_t*)calloc(1, sizeof(usb_device_t));
  if (!newdev) {
    dbus_message_unref(msg);
    return NULL;
  }
  // id
  {
    dbus_uint32_t u = 0;
    dbus_message_iter_get_basic(&iter, &u);
    newdev->id = u;
  }
  dbus_message_iter_next(&iter);
  // event
  {
    dbus_uint32_t u = 0;
    dbus_message_iter_get_basic(&iter, &u);
    newdev->event = u;
  }
  dbus_message_iter_next(&iter);
  // target
  {
    dbus_uint32_t u = 0;
    dbus_message_iter_get_basic(&iter, &u);
    newdev->target = u;
  }
  dbus_message_iter_next(&iter);
  // rule (string)
  {
    const char *rule = NULL;
    dbus_message_iter_get_basic(&iter, &rule);
    safe_strcpy(newdev->device_rule, sizeof(newdev->device_rule), rule);
  }
  dbus_message_iter_next(&iter);
  // properties a{ss}
  if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY) {
    DBusMessageIter arrayIter;
    dbus_message_iter_recurse(&iter, &arrayIter);
    int count = 0;
    while (dbus_message_iter_get_arg_type(&arrayIter) == DBUS_TYPE_DICT_ENTRY) {
      if (count >= MAX_PROPERTIES) {
        DBusMessageIter skip;
        dbus_message_iter_recurse(&arrayIter, &skip);
        dbus_message_iter_next(&arrayIter);
        continue;
      }
      DBusMessageIter dictEntry;
      dbus_message_iter_recurse(&arrayIter, &dictEntry);
      // key
      if (dbus_message_iter_get_arg_type(&dictEntry) != DBUS_TYPE_STRING) {
        dbus_message_iter_next(&arrayIter);
        continue;
      }
      const char *key = NULL;
      dbus_message_iter_get_basic(&dictEntry, &key);
      dbus_message_iter_next(&dictEntry);
      // value
      if (dbus_message_iter_get_arg_type(&dictEntry) != DBUS_TYPE_STRING) {
        dbus_message_iter_next(&arrayIter);
        continue;
      }
      const char *value = NULL;
      dbus_message_iter_get_basic(&dictEntry, &value);
      safe_strcpy(newdev->properties[count].key,   MAX_KEY_LEN,   key);
      safe_strcpy(newdev->properties[count].value, MAX_VALUE_LEN, value);
      count++;
      dbus_message_iter_next(&arrayIter);
    }
    newdev->property_count = count;
  }
  dbus_message_unref(msg);
  return newdev;
}

// Check if the device is a storage device based on properties
int usbguard_storage_device_check(const usb_device_t *info) {
  if (!info || info->property_count <= 0) return 0;
  for (int i = 0; i < info->property_count; ++i) {
    const char *key = info->properties[i].key;
    const char *value = info->properties[i].value;
    if (!key || !value) continue;
    // 1) deviceClass == "08" (Mass Storage)
    if (strcmp(key, "deviceClass") == 0 && strcmp(value, "08") == 0) return 1;
    // 2) composite interface.*.class == "08"
    if (strncmp(key, "interface.", 10) == 0 && strstr(key, ".class") && strcmp(value, "08") == 0) return 1;
    // 3) name contains "Storage"
    if (strcmp(key, "name") == 0 && strstr(value, "Storage")) return 1;
    // 4) with-interface exact or prefix 08:
    if (strcmp(key, "with-interface") == 0 &&
        (strcmp(value, "08:06:50") == 0 || strncmp(value, "08:", 3) == 0)) return 1;
  }
  return 0;
}

// Print USB device info for debugging
static void usbguard_print_info(const usb_device_t *info) {
    if (!info) return;
    printf("USB Info:\n");
    printf("  ID=%u, Event=%u, Target=%u\n", info->id, info->event, info->target);
    printf("  Rule: %s\n", info->device_rule);
    for (int i = 0; i < info->property_count; ++i) {
      printf("    %s = %s\n", info->properties[i].key, info->properties[i].value);
    }
}

// Handle USBGuard events and enqueue them to the global event queue
static void usbguard_handle(DBusConnection *conn) {
  usb_device_t *newdev = usbguard_listen_event(conn);
  if (!newdev) return;
  int enqueued = 0;
  switch (newdev->event) {
    case 1: // Insert
      enqueue_event(&g_event_queue, newdev, USB_EVT_PLUGIN);
      enqueued = 1;
      break;
    case 3: // Remove
    {
      usb_device_t *dev = device_list_find_by_id(newdev->id);
      if (dev) {
        enqueue_event(&g_event_queue, dev, USB_EVT_PLUGOUT);
      } else {
        printf("[DBUS] Remove for unknown id=%u\n", newdev->id);
      }
      break;
    }
    default: // Present
      break;
  }
  usbguard_print_info(newdev); // print debug info
  if (!enqueued) {
    free(newdev);
  }
}
  
// Close the DBus connection
static void usb_manager_close(DBusConnection *conn) {
  if (!conn) return;
  dbus_connection_close(conn);
  dbus_connection_unref(conn);
}

// Thread function to manage USB events
void* usb_manager_loop(void *arg){
    (void)arg;
    DBusConnection *conn = usbguard_init_connection();
    if (!conn) {
        fprintf(stderr, "usbguard_thread: invalid connection\n");
        return NULL;
    }
    while (dbus_connection_read_write_dispatch(conn, -1)) {
      usbguard_handle(conn);
    }
    usb_manager_close(conn);
    return NULL;
}

// Apply device policy via USBGuard DBus interface
// unsigned int target: 0 = allow, 1 = block, 2 = reject
// bool permanent: true = permanent rule, false = temporary
// out_rule_id: if permanent=true, return the created rule ID here
int apply_device_policy(unsigned int device_id, unsigned int target, bool permanent, unsigned int *out_rule_id) {
    DBusConnection *conn;
    DBusError err;
    DBusMessage *msg, *reply;
    dbus_bool_t dbus_permanent = permanent ? TRUE : FALSE;
    unsigned int rule_id = 0;

    dbus_error_init(&err);
    // Connect to the system bus
    conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "[apply_device_policy] Connection error: %s\n", err.message);
        dbus_error_free(&err);
        return -1;
    }
    if (!conn) {
        fprintf(stderr, "[apply_device_policy] Can not connect system bus\n");
        return -2;
    }
    // create method call
    msg = dbus_message_new_method_call(
        "org.usbguard1",
        "/org/usbguard1/Devices",
        "org.usbguard.Devices1",
        "applyDevicePolicy");
    if (!msg) {
        fprintf(stderr, "[apply_device_policy] Can not create message\n");
        return -3;
    }
    // add arguments to message 
    if (!dbus_message_append_args(msg,
        DBUS_TYPE_UINT32, &device_id,
        DBUS_TYPE_UINT32, &target,
        DBUS_TYPE_BOOLEAN, &dbus_permanent,
        DBUS_TYPE_INVALID))
    {
        fprintf(stderr, "[apply_device_policy] Can not append args\n");
        dbus_message_unref(msg);
        return -4;
    }

    // send message and wait for reply
    reply = dbus_connection_send_with_reply_and_block(conn, msg, -1, &err);
    dbus_message_unref(msg);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "[apply_device_policy] Message call error: %s\n", err.message);
        dbus_error_free(&err);
        return -5;
    }
    if (!reply) {
        fprintf(stderr, "[apply_device_policy] No response fron d-bus error\n");
        return -6;
    }
    // read rule if permanent = true
    if (permanent) {
        if (!dbus_message_get_args(reply, &err,
                                   DBUS_TYPE_UINT32, &rule_id,
                                   DBUS_TYPE_INVALID))
        {
            fprintf(stderr, "[apply_device_policy] Can not read rule_id: %s\n", err.message);
            dbus_error_free(&err);
            dbus_message_unref(reply);
            return -7;
        }
        if (out_rule_id) *out_rule_id = rule_id;
    }
    dbus_message_unref(reply);
    return 0; // success
}