#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "driver/uart.h"
#include "string.h"
#include "my_global_lib.h"
#include "usr_k2_lan.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "../ui/ui.h"
#include "../ui/safe_page.h"
#include "nvs_helper.h"
#include "my_global_lib.h"
#include "transaction.h"

void execute_info(char fields[MAX_FIELDS][MAX_FIELD_LEN])
{

    // <GETINFO|${is_active}|${is_production}|${unit_sn}|${currency}|${amount}|${location}|${area}|${ovo_tid}>
    char *is_active = fields[1];
    char *is_production = fields[2];
    char *unit_sn = fields[3];
    char *currency = fields[4];
    char *amount = fields[5];
    char *location = fields[6];
    char *area = fields[7];
    char *ovo_tid = fields[8];

    if (is_active) {
        strncpy(DEVICE_INFO.IsActive, is_active, sizeof(DEVICE_INFO.IsActive) - 1);
        DEVICE_INFO.IsActive[sizeof(DEVICE_INFO.IsActive) - 1] = '\0';
    }
    if (is_production) {
        strncpy(DEVICE_INFO.IsProduction, is_production, sizeof(DEVICE_INFO.IsProduction) - 1);
        DEVICE_INFO.IsProduction[sizeof(DEVICE_INFO.IsProduction) - 1] = '\0';
    }
    if (unit_sn) {
        strncpy(DEVICE_INFO.UnitSn, unit_sn, sizeof(DEVICE_INFO.UnitSn) - 1);
        DEVICE_INFO.UnitSn[sizeof(DEVICE_INFO.UnitSn) - 1] = '\0';
    }
    if (currency) {
        strncpy(DEVICE_INFO.CurCode, currency, sizeof(DEVICE_INFO.CurCode) - 1);
        DEVICE_INFO.CurCode[sizeof(DEVICE_INFO.CurCode) - 1] = '\0';
    }
    if (amount) {
        strncpy(DEVICE_INFO.GrossAmountTc, amount, sizeof(DEVICE_INFO.GrossAmountTc) - 1);
        DEVICE_INFO.GrossAmountTc[sizeof(DEVICE_INFO.GrossAmountTc) - 1] = '\0';
    }
    if (location) {
        strncpy(DEVICE_INFO.Location, location, sizeof(DEVICE_INFO.Location) - 1);
        DEVICE_INFO.Location[sizeof(DEVICE_INFO.Location) - 1] = '\0';
    }
    if (area) {
        strncpy(DEVICE_INFO.Area, area, sizeof(DEVICE_INFO.Area) - 1);
        DEVICE_INFO.Area[sizeof(DEVICE_INFO.Area) - 1] = '\0';
    }
    if (ovo_tid) {
        strncpy(DEVICE_INFO.OvoTid, ovo_tid, sizeof(DEVICE_INFO.OvoTid) - 1);
        DEVICE_INFO.OvoTid[sizeof(DEVICE_INFO.OvoTid) - 1] = '\0';
    }

    save_nvs_str("storage", "IsActive", is_active);
    save_nvs_str("storage", "IsProduction", is_production);
    save_nvs_str("storage", "UnitSn", unit_sn);
    save_nvs_str("storage", "CurCode", currency);
    save_nvs_str("storage", "GrossAmountTc", amount);
    save_nvs_str("storage", "Location", location);
    save_nvs_str("storage", "Area", area);
    save_nvs_str("storage", "OvoTid", ovo_tid);

    DEBUG_PRINTLN("Info is uptodate!");
}

void execute_ordercheck(char fields[MAX_FIELDS][MAX_FIELD_LEN])
{
    char *order_status = fields[1];
    if (strcmp(order_status, "E") == 0)
    { // expired
        PAYMENT_ROUTE.IsPaymentSuccess = false;
    }
    else if (strcmp(order_status, "S") == 0)
    { // settlement
        PAYMENT_ROUTE.IsPaymentSuccess = true;
    }
    else if (strcmp(order_status, "P") == 0)
    { // pending
      // dont set payment route because, this value is repetitive
    }
    else if (strcmp(order_status, "D") == 0)
    { // deny
        PAYMENT_ROUTE.IsPaymentSuccess = false;
    }
    else if (strcmp(order_status, "C") == 0)
    { // cancel
        PAYMENT_ROUTE.IsPaymentSuccess = false;
    }
    else if (strcmp(order_status, "F") == 0)
    { // failure
        PAYMENT_ROUTE.IsPaymentSuccess = false;
    }
}

void execute_payment(char fields[MAX_FIELDS][MAX_FIELD_LEN])
{
    // <PAYMENT|${order_id}|${qr_string}>
    // reset flag
    PAYMENT_ROUTE.IsPaymentSuccess = false;

    // save order id
    save_nvs_str("storage", "OrderId", fields[1]);
    save_nvs_str("storage", "QrisImage", fields[2]);
    vTaskDelay(pdMS_TO_TICKS(1000));
    lv_async_call(go_page8, NULL);
}

void execute_addtransaction(char fields[MAX_FIELDS][MAX_FIELD_LEN])
{
    DEBUG_PRINTLN("ADDTRANSACTION  : %s", fields[2]);
}

void execute_else()
{
    DEBUG_PRINTLN("ELSE");
}

bool is_all_ascii(const char *data)
{
    while (*data)
    {
        if ((unsigned char)*data > 127)
            return false;
        data++;
    }
    return true;
}