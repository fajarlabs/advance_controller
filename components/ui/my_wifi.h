#ifndef MY_WIFI_H
#define MY_WIFI_H

#include "esp_err.h"

// Callback function type untuk update label
typedef void (*wifi_status_callback_t)(const char *message, int color_r, int color_g, int color_b);

void my_wifi_init(const char *ssid, const char *password);
int my_wifi_is_connected();
void my_wifi_reconnect();
void my_wifi_connect();
void my_wifi_disconnect();
char *my_wifi_send_getinfo_request(const char *url, const char *post_data);
void make_http_request(const char *prepared_req_deviceid);

// Function untuk set callback
void my_wifi_set_status_callback(wifi_status_callback_t callback);
// Function untuk update status dari luar
void my_wifi_update_status(const char *message, int color_r, int color_g, int color_b);

#endif