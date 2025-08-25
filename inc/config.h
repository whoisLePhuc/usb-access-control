#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>
#include "logger/logger.h"

// Configuration structure
typedef struct {
    char ca_cert_path[256];
    char log_file_path[256];
    int log_level;
    int event_queue_size;
    int device_timeout_seconds;
    int cert_verify_timeout_seconds;
    int usb_sig_retry_count;
    int usb_sig_retry_delay_ms;
} app_config_t;

// Default configuration values
#define DEFAULT_CA_CERT_PATH "/home/lephuc/usb-access-control/cert/ca.crt"
#define DEFAULT_LOG_FILE_PATH "/home/lephuc/usb-access-control/log_file.txt"
#define DEFAULT_LOG_LEVEL LOG_DEBUG
#define DEFAULT_EVENT_QUEUE_SIZE 64
#define DEFAULT_DEVICE_TIMEOUT_SECONDS 30
#define DEFAULT_CERT_VERIFY_TIMEOUT_SECONDS 10
#define DEFAULT_USB_SIG_RETRY_COUNT 5
#define DEFAULT_USB_SIG_RETRY_DELAY_MS 1000

// Configuration functions
int config_init(const char *config_file);
void config_cleanup(void);
const app_config_t* config_get(void);

// Environment variable overrides
int config_load_from_env(void);

#endif // CONFIG_H
