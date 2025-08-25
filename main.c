#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include "core/usb_queue.h"
#include "fsm/usb_fsm.h"
#include "core/usb_thread.h"
#include "interface/usbguard_interface.h"
#include "core/usb_device.h"
#include "config.h"
#include "logger/logger.h"

event_queue_t g_event_queue; // global so FSM can enqueue follow-ups
static volatile int g_shutdown_requested = 0; // Global flag for graceful shutdown
// Signal handler for graceful shutdown
static void signal_handler(int sig) {
    (void)sig;
    g_shutdown_requested = 1;
    logMessage(LOG_INFO, "Shutdown signal received. Cleaning up...");
}

int main(void) {
    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    // Initialize configuration
    if (config_init(NULL) != 0) {
        logMessage(LOG_WARNING, "Failed to initialize configuration");
        return 1;
    }
    // Initialize logger
    const app_config_t *config = config_get();
    if (config) {
        loggerInit(config->log_level, config->log_file_path);
        logMessage(LOG_INFO, "Logger initialized with level %d, file: %s", config->log_level, config->log_file_path);
    } else {
        // Fallback to default logger settings
        loggerInit(LOG_INFO, "log_file.txt");
        logMessage(LOG_WARNING, "Using default logger settings");
    }
    // Initialize FSM
    if (fsm_init() != 0) {
        logMessage(LOG_WARNING, "Failed to initialize FSM");
        config_cleanup();
        return 1;
    }
    // Initialize event queue
    if (queue_init(&g_event_queue) != 0) {
        logMessage(LOG_WARNING, "Failed to initialize event queue");
        config_cleanup();
        return 1;
    }
    // Create FSM thread
    pthread_t fsm_thread;
    if (pthread_create(&fsm_thread, NULL, fsm_loop, &g_event_queue) != 0) {
        fprintf(stderr, "Failed to create FSM thread: %s\n", strerror(errno));
        queue_destroy(&g_event_queue);
        config_cleanup();
        return 1;
    }
    // Create USB manager thread
    pthread_t usb_manager_thread;
    if (pthread_create(&usb_manager_thread, NULL, usb_manager_loop, NULL) != 0) {
        logMessage(LOG_WARNING, "Failed to create USB manager thread");
        pthread_cancel(fsm_thread);
        pthread_join(fsm_thread, NULL);
        queue_destroy(&g_event_queue);
        config_cleanup();
        return 1;
    }
    logMessage(LOG_INFO, "USB Access Control started successfully");
    // Wait for shutdown signal
    while (!g_shutdown_requested) {
        sleep(1);
    }
    logMessage(LOG_INFO, "Shutting down...");
    enqueue_event(&g_event_queue, NULL, USB_EVT_EXIT);
    int wait_attempts = 20; // 20 x 100ms = 2s
    while (wait_attempts-- > 0) {
        usleep(100000); // 100 ms
    }
    int cancel_needed = 0;
    if (pthread_kill(usb_manager_thread, 0) == 0) {
        pthread_cancel(usb_manager_thread);
        cancel_needed = 1;
    }
    if (pthread_kill(fsm_thread, 0) == 0) {
        pthread_cancel(fsm_thread);
        cancel_needed = 1;
    }
    if (cancel_needed) {
        logMessage(LOG_WARNING, "Forcing thread cancellation...");
    }
    pthread_join(usb_manager_thread, NULL);
    pthread_join(fsm_thread, NULL);
    queue_destroy(&g_event_queue);
    if (!device_list_is_empty()) {
        logMessage(LOG_WARNING, "Device list not empty on exit. Force cleanup...");
        device_list_force_cleanup();
    }
    config_cleanup();
    loggerClose();
    logMessage(LOG_INFO, "Cleanup completed");
    return 0;
}
