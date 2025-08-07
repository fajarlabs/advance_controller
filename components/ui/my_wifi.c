#include "my_wifi.h"
#include "my_global_lib.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include <string.h>
#include "esp_http_client.h"
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "transaction.h"
#include "nvs_helper.h"
#include "my_global_lib.h"
#include "usr_k2_lan.h"
#include "safe_page.h"
#include "lvgl.h"
#include "ui.h"

// static const char *TAG = "MY_WIFI";

static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

// Callback function pointer untuk update status
static wifi_status_callback_t status_callback = NULL;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        DEBUG_PRINTLN("Disconnected. Reconnecting...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        DEBUG_PRINTLN("Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void my_wifi_init(const char *ssid, const char *password)
{
    // Initialize NVS (if not done in app_main)
    nvs_flash_init();

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_event_group = xEventGroupCreate();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    //ESP_LOGI(TAG, "Connecting to WiFi: %s", ssid);

    // Wait for connection
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                           WIFI_CONNECTED_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(10000)); // wait max 10s

    if (bits & WIFI_CONNECTED_BIT) {
        DEBUG_PRINTLN("Wifi Connected to %s", ssid);
        //ESP_LOGI(TAG, "WiFi Connected.");
    } else {
        DEBUG_PRINTLN("WiFi connection failed or timed out.");
        //ESP_LOGW(TAG, "WiFi connection timeout.");
    }
}

int my_wifi_is_connected()
{
    wifi_ap_record_t ap_info;
    return esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK;
}

void my_wifi_reconnect()
{
    DEBUG_PRINTLN("Reconnect WiFi...");
    esp_wifi_disconnect();
    esp_wifi_connect();
}

void my_wifi_connect(){
    esp_wifi_connect();
}

void my_wifi_disconnect(){
    esp_wifi_disconnect();
}

char *my_wifi_send_getinfo_request(const char *url, const char *post_data)
{
    // Buffer statis untuk menyimpan response
    static char buffer[2048];
    memset(buffer, 0, sizeof(buffer));

    if (!url) {
        DEBUG_PRINTLN("URL is NULL");
        return NULL;
    }

    // Automatically append /endpoint to the URL if not already present
    static char full_url[128];
    memset(full_url, 0, sizeof(full_url));
    
    // Check if URL already contains /endpoint
    if (strstr(url, "/endpoint") != NULL) {
        // URL already has endpoint, use as is
        strncpy(full_url, url, sizeof(full_url) - 1);
    } else {
        // Append /endpoint to the URL
        snprintf(full_url, sizeof(full_url), "%s/endpoint", url);
    }
    
    DEBUG_PRINTLN("Original URL: %s", url);
    DEBUG_PRINTLN("Full URL with endpoint: %s", full_url);
    
    // Use full_url instead of url from now on
    url = full_url;

    // Check if WiFi is connected before making HTTP request
    if (my_wifi_is_connected() != 1) {
        DEBUG_PRINTLN("WiFi not connected, cannot make HTTP request");
        return NULL;
    }

    // Add longer delay to ensure network stack is fully ready
    DEBUG_PRINTLN("Waiting for network stack to be ready...");
    vTaskDelay(pdMS_TO_TICKS(2000)); // Wait 2 seconds

    // Test basic network connectivity with ping-like check
    DEBUG_PRINTLN("Testing network connectivity...");
    
    // Extract host from URL for connection testing
    char host[64] = {0};
    if (strncmp(full_url, "http://", 7) == 0) {
        const char *host_start = full_url + 7;
        const char *host_end = strchr(host_start, ':');
        if (!host_end) host_end = strchr(host_start, '/');
        if (!host_end) host_end = host_start + strlen(host_start);
        
        int host_len = host_end - host_start;
        if (host_len < sizeof(host)) {
            strncpy(host, host_start, host_len);
            host[host_len] = '\0';
        }
    }
    
    DEBUG_PRINTLN("Extracted host: %s", host);

    // Konfigurasi HTTP client - perbaikan untuk response reading
    esp_http_client_config_t config = {
        .url = full_url,  // Use full_url with endpoint
        .method = HTTP_METHOD_POST,
        .timeout_ms = 30000,        // Increased timeout to 30 seconds
        .buffer_size = 1024,        // Buffer internal untuk chunked reading
        .buffer_size_tx = 512,
        .disable_auto_redirect = false,
        .max_redirection_count = 3,
        .is_async = false,          // Pastikan synchronous
        .skip_cert_common_name_check = true,  // Skip certificate verification
        .use_global_ca_store = false,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        DEBUG_PRINTLN("Failed to initialize HTTP client");
        return NULL;
    }

    // Set headers
    esp_http_client_set_header(client, "Content-Type", "text/plain");
    esp_http_client_set_header(client, "User-Agent", "ESP32-HTTP-Client");
    esp_http_client_set_header(client, "Accept", "*/*");
    esp_http_client_set_header(client, "Cache-Control", "no-cache");

    // Set POST data jika ada
    if (post_data && strlen(post_data) > 0) {
        esp_http_client_set_post_field(client, post_data, strlen(post_data));
        DEBUG_PRINTLN("POST data length: %d", strlen(post_data));
    }

    DEBUG_PRINTLN("Starting HTTP request to: %s", full_url);

    // PERBAIKAN: Gunakan esp_http_client_open terlebih dahulu
    DEBUG_PRINTLN("Attempting to open HTTP connection...");
    esp_err_t err = esp_http_client_open(client, post_data ? strlen(post_data) : 0);
    if (err != ESP_OK) {
        DEBUG_PRINTLN("Failed to open HTTP connection: %s (0x%x)", esp_err_to_name(err), err);
        
        // Additional error information
        if (err == ESP_ERR_HTTP_CONNECT) {
            DEBUG_PRINTLN("HTTP connection failed - possible causes:");
            DEBUG_PRINTLN("1. Server is not reachable");
            DEBUG_PRINTLN("2. Wrong IP address or port");
            DEBUG_PRINTLN("3. Firewall blocking connection");
            DEBUG_PRINTLN("4. Network connectivity issues");
        }
        
        // WiFi terhubung tapi tidak ada internet, arahkan ke halaman tidak ada internet
        DEBUG_PRINTLN("WiFi connected but no internet access. Redirecting to no internet page...");
        
        // Update status melalui callback jika tersedia
        if (status_callback) {
            DEBUG_PRINTLN("Calling status callback with message: Internet terputus");
            status_callback("Internet terputus", 255, 165, 0); // Orange untuk internet terputus
        } else {
            DEBUG_PRINTLN("Status callback is NULL, cannot update label");
        }
        
        esp_http_client_cleanup(client);
        lv_async_call(go_page4, NULL);
        return NULL;
    }
    
    DEBUG_PRINTLN("HTTP connection opened successfully");

    // Write POST data jika ada
    if (post_data && strlen(post_data) > 0) {
        int wlen = esp_http_client_write(client, post_data, strlen(post_data));
        if (wlen < 0) {
            DEBUG_PRINTLN("Write POST data failed");
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
            return NULL;
        }
        DEBUG_PRINTLN("Written %d bytes of POST data", wlen);
    }

    // Fetch headers - ini akan mengirim request dan menerima response headers
    int64_t content_length = esp_http_client_fetch_headers(client);
    if (content_length < 0) {
        DEBUG_PRINTLN("Failed to fetch headers");
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return NULL;
    }

    // Get status code
    int status_code = esp_http_client_get_status_code(client);
    DEBUG_PRINTLN("HTTP Status Code: %d", status_code);
    DEBUG_PRINTLN("Content-Length: %lld", content_length);

    // Check if request was successful
    if (status_code < 200 || status_code >= 300) {
        DEBUG_PRINTLN("HTTP request failed with status: %d", status_code);
        
        // Provide specific guidance for common HTTP status codes
        switch (status_code) {
            case 404:
                DEBUG_PRINTLN("Error 404: Not Found - Check if the URL endpoint is correct");
                DEBUG_PRINTLN("Current URL: %s", full_url);
                DEBUG_PRINTLN("Make sure the server has the correct endpoint path");
                break;
            case 403:
                DEBUG_PRINTLN("Error 403: Forbidden - Server rejected the request");
                break;
            case 500:
                DEBUG_PRINTLN("Error 500: Internal Server Error - Server-side issue");
                break;
            case 502:
                DEBUG_PRINTLN("Error 502: Bad Gateway - Server connectivity issue");
                break;
            case 503:
                DEBUG_PRINTLN("Error 503: Service Unavailable - Server temporarily unavailable");
                break;
            default:
                DEBUG_PRINTLN("HTTP error %d - Check server configuration", status_code);
                break;
        }
        
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return NULL;
    }

    // Read response data - PERBAIKAN: gunakan esp_http_client_read
    int total_read = 0;
    int buffer_size = sizeof(buffer) - 1;

    while (total_read < buffer_size) {
        int data_read = esp_http_client_read(client, 
                                           buffer + total_read, 
                                           buffer_size - total_read);
        
        if (data_read < 0) {
            DEBUG_PRINTLN("Error reading response data: %d", data_read);
            break;
        } else if (data_read == 0) {
            DEBUG_PRINTLN("Finished reading response data");
            break;
        }

        total_read += data_read;
        DEBUG_PRINTLN("Read %d bytes, total: %d bytes", data_read, total_read);
        
        // Jika sudah membaca sesuai content_length, berhenti
        if (content_length > 0 && total_read >= content_length) {
            DEBUG_PRINTLN("Read complete according to content-length");
            break;
        }
    }

    // Null-terminate the response
    buffer[total_read] = '\0';

    // Cleanup
    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    if (total_read > 0) {
        DEBUG_PRINTLN("HTTP Response (%d bytes):", total_read);
        DEBUG_PRINTLN("%s", buffer);
        return buffer;
    } else {
        DEBUG_PRINTLN("No response data received");
        return NULL;
    }
}

void make_http_request(const char *prepared_req_deviceid)
{
    static char server_host[30];
    const char *nvs_namespace = "storage";
    
    // Wait for WiFi to be connected before making HTTP request
    DEBUG_PRINTLN("Waiting for WiFi connection...");
    int retry_count = 0;
    const int max_retries = 5; // Wait up to 5 seconds

    while (my_wifi_is_connected() != 1 && retry_count < max_retries)
    {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Wait 1 second
        retry_count++;
        DEBUG_PRINTLN("WiFi connection attempt %d/%d", retry_count, max_retries);
    }

    if (my_wifi_is_connected() != 1)
    {
        DEBUG_PRINTLN("WiFi connection timeout! Could not connect after %d seconds", max_retries);
        DEBUG_PRINTLN("Redirecting to page 4 due to internet connection issues...");
        
        // Update status melalui callback jika tersedia
        if (status_callback) {
            DEBUG_PRINTLN("Calling status callback with message: Internet terputus");
            status_callback("Internet terputus", 255, 165, 0); // Orange untuk internet terputus
        } else {
            DEBUG_PRINTLN("Status callback is NULL, cannot update label");
        }
        
        lv_async_call(go_page4, NULL);
        return;
    }
    
    DEBUG_PRINTLN("WiFi connected! Making HTTP request...");
    memset(server_host, 0, sizeof(server_host));
    load_nvs_str(nvs_namespace, "server_host", server_host, sizeof(server_host));

    DEBUG_PRINTLN("Making request to: %s (will auto-append /endpoint)", server_host);
    char *response_data = my_wifi_send_getinfo_request(server_host, prepared_req_deviceid);
    
    if (response_data && is_all_ascii((char *)response_data))
    {
        parse_uart_response((char *)response_data);
    }
    else
    {
        DEBUG_PRINTLN("Non-ASCII data received or no response, skipping parse.");
    }
}

// Function untuk set callback
void my_wifi_set_status_callback(wifi_status_callback_t callback)
{
    DEBUG_PRINTLN("Setting WiFi status callback: %s", callback ? "Valid" : "NULL");
    status_callback = callback;
}

// Function untuk update status dari luar
void my_wifi_update_status(const char *message, int color_r, int color_g, int color_b)
{
    DEBUG_PRINTLN("my_wifi_update_status called with message: %s", message);
    if (status_callback) {
        DEBUG_PRINTLN("Calling status callback from update function");
        status_callback(message, color_r, color_g, color_b);
    } else {
        DEBUG_PRINTLN("Status callback is NULL in update function");
    }
}