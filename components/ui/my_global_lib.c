#include "my_global_lib.h"
#include <stdarg.h>

// Definisi global variables
ParseInfo DEVICE_INFO = {0};
QrisCpmData QRIS_CPM = {0};
PaymentRoute PAYMENT_ROUTE = {0};

// Debug control variable - can be changed at runtime
static bool debug_enabled = DEBUG_MODE;

// Function to enable/disable debug at runtime
void set_debug_mode(bool enable) {
    debug_enabled = enable;
}

// Function to get current debug status
bool get_debug_mode(void) {
    return debug_enabled;
}

// Runtime debug print functions
void debug_printf(const char *format, ...) {
    if (debug_enabled) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
}

void debug_println(const char *format, ...) {
    if (debug_enabled) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        printf("\n");
    }
}
