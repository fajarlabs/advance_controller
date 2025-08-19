#ifndef MY_GLOBAL_LIB_H
#define MY_GLOBAL_LIB_H

#include <stdbool.h>
#include <stdio.h>

// Debug configuration - Set to 1 to enable debug prints, 0 to disable
#define DEBUG_MODE 0
#define DEBUG_TRANSACTION_FLOW 1
#define IS_PRODUCTION 0

// Debug print macros
#if DEBUG_MODE
    #define DEBUG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
    #define DEBUG_PRINTLN(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(fmt, ...) ((void)0)
    #define DEBUG_PRINTLN(fmt, ...) ((void)0)
#endif

// Struktur untuk menyimpan informasi perangkat
typedef struct {
    char IsActive[16];
    char IsProduction[16];
    char UnitSn[64];
    char CurCode[16];
    char GrossAmountTc[32];
    char Location[64];
    char Area[64];
    char OvoTid[64];
} ParseInfo;

// Struktur untuk data QRIS CPM
typedef struct {
    char ORDER_ID[100];
    char QR_IMAGE[300];
} QrisCpmData;

// Struktur untuk status dan jenis pembayaran
typedef struct {
    int PaymentType;
    bool IsPaymentSuccess;
} PaymentRoute;

// Network connection type enum
typedef enum {
    NETWORK_UNDEFINED = 0,
    NETWORK_LAN = 1,
    NETWORK_WIFI = 2
} network_type_t;

typedef struct
{
    char device_id[64];
    char unit_sn[64];
    int gross_price;
} transaction_params_t;

// Deklarasi variabel global
extern ParseInfo DEVICE_INFO;
extern QrisCpmData QRIS_CPM;
extern PaymentRoute PAYMENT_ROUTE;

// Debug function declarations
void set_debug_mode(bool enable);
bool get_debug_mode(void);
void debug_printf(const char *format, ...);
void debug_println(const char *format, ...);

#endif // MY_GLOBAL_LIB_H