#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "nvs_helper.h"
#include "my_global_lib.h"
#include <string.h>

const char* TAG = "NVS_HELPER";

void init_nvs()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

esp_err_t save_nvs_i32(const char *namespace, const char *key, int32_t value)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace, NVS_READWRITE, &handle);
    if (err != ESP_OK)
        return err;

    err = nvs_set_i32(handle, key, value);
    if (err == ESP_OK)
        err = nvs_commit(handle);

    nvs_close(handle);
    return err;
}

esp_err_t save_nvs_str(const char *namespace, const char *key, const char *value)
{
    if (!namespace || !key || !value) {
        return ESP_ERR_INVALID_ARG;  // Cek null pointer
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_str(handle, key, value);
    if (err == ESP_OK) {
        err = nvs_commit(handle);  // Commit hanya jika set berhasil
    }

    nvs_close(handle);  // Tetap close meskipun commit gagal
    return err;
}

esp_err_t load_nvs_str(const char *namespace, const char *key, char *out_value, size_t out_value_size) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace, NVS_READONLY, &handle);
    if (err != ESP_OK)
        return err;

    size_t required_size = 0;
    err = nvs_get_str(handle, key, NULL, &required_size); // Dapatkan panjang string
    if (err != ESP_OK) {
        nvs_close(handle);
        return err;
    }

    if (required_size > out_value_size) {
        nvs_close(handle);
        return ESP_ERR_NVS_VALUE_TOO_LONG;
    }

    err = nvs_get_str(handle, key, out_value, &required_size);
    nvs_close(handle);
    return err;
}

esp_err_t load_nvs_i32(const char *namespace, const char *key, int32_t *out_value)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace, NVS_READONLY, &handle);
    if (err != ESP_OK)
        return err;

    err = nvs_get_i32(handle, key, out_value);
    nvs_close(handle);
    return err;
}

void load_nvs_data()
{
    // Load Money
    int32_t valueMoney;
    // Load Pulse
    int32_t valuePulse;
    // Load Money
    int32_t valueDuty;
    // Load Pulse
    int32_t valueDuration;
    // Load QRPulse
    int32_t valueQRPulse;
    // Rate Conversion atau Konversi Pulsa terhadal uang yang masuk 
    // di bill acceptor yang telah di setting
    int32_t valueRateConversion;
    // timeout bill
    int32_t valueExpiredTimeBill;
    // is wifi or lan
    int32_t isWifiOrLan;


    if (load_nvs_i32("storage", "money", &valueMoney) == ESP_OK)
    {
        DEBUG_PRINTLN("Loaded money: %d", (int)valueMoney);
    } else {
        valueMoney = 0;
    }
    if (load_nvs_i32("storage", "pulse", &valuePulse) == ESP_OK)
    {
        DEBUG_PRINTLN("Loaded pulse: %d", (int)valuePulse);
    } else {
        valuePulse = 0;
    }

    if (load_nvs_i32("storage", "duty", &valueDuty) == ESP_OK)
    {
        DEBUG_PRINTLN("Loaded duty: %d", (int)valueDuty);
    } else {
        valueDuty = 0;
    }

    if (load_nvs_i32("storage", "duration", &valueDuration) == ESP_OK)
    {
        DEBUG_PRINTLN("Loaded duration: %d", (int)valueDuration);
    } else {
        valueDuration = 0; 
    }
    if (load_nvs_i32("storage", "qrpulse", &valueQRPulse) == ESP_OK)
    {
        DEBUG_PRINTLN("Loaded qrpulse: %d", (int)valueQRPulse);
    } else {
        valueQRPulse = 0;
    }
    if (load_nvs_i32("storage", "RateConversion", &valueRateConversion) == ESP_OK)
    {
        DEBUG_PRINTLN("Loaded RateConversion: %d", (int)valueRateConversion);
    } else {
        valueRateConversion = 0;
    }
    if (load_nvs_i32("storage", "ExpiredTimeBill", &valueExpiredTimeBill) == ESP_OK)
    {
        DEBUG_PRINTLN("Loaded ExpiredTimeBill: %d", (int)valueExpiredTimeBill);
    } else {
        valueExpiredTimeBill = 0;
    }

    // wifi access point
    char wifi_ap[64];
    if (load_nvs_str("storage", "wifi_ap", wifi_ap, sizeof(wifi_ap)) == ESP_OK) {
        DEBUG_PRINTLN("Loaded WiFi AP: %s", wifi_ap);
    } else {
        strncpy(wifi_ap, "NOT SET", sizeof(wifi_ap));
        wifi_ap[sizeof(wifi_ap) - 1] = '\0';  // pastikan null-terminated
    }

    char pass_ap[64];
    if (load_nvs_str("storage", "pass_ap", pass_ap, sizeof(pass_ap)) == ESP_OK)
    {
        DEBUG_PRINTLN("Loaded pass_ap: %s", pass_ap);
    } else {
        strncpy(pass_ap, "NOT SET", sizeof(pass_ap));
        pass_ap[sizeof(pass_ap) - 1] = '\0';  // pastikan null-terminated
    }

    char server_host[64];
    if (load_nvs_str("storage", "server_host", server_host, sizeof(server_host)) == ESP_OK)
    {
        DEBUG_PRINTLN("Loaded server_host: %s", server_host);
    } else {
        strncpy(server_host, "NOT SET", sizeof(server_host));
        server_host[sizeof(server_host) - 1] = '\0';  // pastikan null-terminated
    }

    if (load_nvs_i32("storage", "is_wifiorlan", &isWifiOrLan) == ESP_OK)
    {
        DEBUG_PRINTLN("Loaded is_wifiorlan: %d", (int)isWifiOrLan);
    } else {
        isWifiOrLan = 1;
    }

    printf("<s>10002,%d,%d,%d,%d,%d,%d,%d,%s,%s,%s,%d<e>\n",
        (int)valueMoney,
        (int)valuePulse,
        (int)valueDuty,
        (int)valueDuration,
        (int)valueQRPulse,
        (int)valueRateConversion,
        (int)valueExpiredTimeBill,
        wifi_ap,
        pass_ap,
        server_host,
        (int)isWifiOrLan);
}