#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "config.h"

static app_config_t g_config;
static int g_config_initialized = 0;

// Initialize configuration with defaults
static void config_set_defaults(void) {
    strncpy(g_config.ca_cert_path, DEFAULT_CA_CERT_PATH, sizeof(g_config.ca_cert_path) - 1);
    strncpy(g_config.log_file_path, DEFAULT_LOG_FILE_PATH, sizeof(g_config.log_file_path) - 1);
    g_config.log_level = DEFAULT_LOG_LEVEL;
    g_config.event_queue_size = DEFAULT_EVENT_QUEUE_SIZE;
    g_config.device_timeout_seconds = DEFAULT_DEVICE_TIMEOUT_SECONDS;
    g_config.cert_verify_timeout_seconds = DEFAULT_CERT_VERIFY_TIMEOUT_SECONDS;
    g_config.usb_sig_retry_count = DEFAULT_USB_SIG_RETRY_COUNT;
    g_config.usb_sig_retry_delay_ms = DEFAULT_USB_SIG_RETRY_DELAY_MS;
}

// Load configuration from environment variables
int config_init(const char *config_file) {
    if (g_config_initialized) {
        return 0; // Already initialized
    }
    config_set_defaults();
    if (config_load_from_env() != 0) {
        fprintf(stderr, "Warning: Failed to load environment variables, using defaults\n");
    }
    g_config_initialized = 1;
    return 0;
}

// Cleanup configuration resources if any
void config_cleanup(void) {
    g_config_initialized = 0;
}

// Get a pointer to the current configuration
const app_config_t* config_get(void) {
    if (!g_config_initialized) {
        fprintf(stderr, "Error: Configuration not initialized\n");
        return NULL;
    }
    return &g_config;
}

// Load configuration from environment variables
int config_load_from_env(void) {
    const char *ca_path = getenv("USB_ACCESS_CA_CERT_PATH");
    if (ca_path && strlen(ca_path) > 0) {
        if (strlen(ca_path) >= sizeof(g_config.ca_cert_path)) {
            fprintf(stderr, "Error: CA_CERT_PATH too long\n");
            return -1;
        }
        strncpy(g_config.ca_cert_path, ca_path, sizeof(g_config.ca_cert_path) - 1);
    }
    const char *log_path = getenv("USB_ACCESS_LOG_PATH");
    if (log_path && strlen(log_path) > 0) {
        if (strlen(log_path) >= sizeof(g_config.log_file_path)) {
            fprintf(stderr, "Error: LOG_PATH too long\n");
            return -1;
        }
        strncpy(g_config.log_file_path, log_path, sizeof(g_config.log_file_path) - 1);
    }
    const char *log_level = getenv("USB_ACCESS_LOG_LEVEL");
    if (log_level) {
        char *endptr;
        long level = strtol(log_level, &endptr, 10);
        if (*endptr == '\0' && level >= 0 && level <= 5) {
            g_config.log_level = (int)level;
        } else {
            fprintf(stderr, "Warning: Invalid LOG_LEVEL, using default\n");
        }
    }
    const char *queue_size = getenv("USB_ACCESS_QUEUE_SIZE");
    if (queue_size) {
        char *endptr;
        long size = strtol(queue_size, &endptr, 10);
        if (*endptr == '\0' && size > 0 && size <= 1000) {
            g_config.event_queue_size = (int)size;
        } else {
            fprintf(stderr, "Warning: Invalid QUEUE_SIZE, using default\n");
        }
    }
    const char *retry_count = getenv("USB_ACCESS_SIG_RETRY_COUNT");
    if (retry_count) {
        char *endptr;
        long count = strtol(retry_count, &endptr, 10);
        if (*endptr == '\0' && count > 0 && count <= 20) {
            g_config.usb_sig_retry_count = (int)count;
        } else {
            fprintf(stderr, "Warning: Invalid SIG_RETRY_COUNT, using default\n");
        }
    }
    const char *retry_delay = getenv("USB_ACCESS_SIG_RETRY_DELAY_MS");
    if (retry_delay) {
        char *endptr;
        long delay = strtol(retry_delay, &endptr, 10);
        if (*endptr == '\0' && delay >= 100 && delay <= 10000) {
            g_config.usb_sig_retry_delay_ms = (int)delay;
        } else {
            fprintf(stderr, "Warning: Invalid SIG_RETRY_DELAY_MS, using default\n");
        }
    }
    return 0;
}
