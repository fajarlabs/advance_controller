#include <stdio.h>
#include <unistd.h>
#include <sys/lock.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "lvgl.h"
#include "../ui/ui.h"
#include "../ui/safe_page.h"
#include "../ui/my_global_lib.h"
#include <stdio.h>
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "driver/uart.h"
#include "nvs_helper.h"
#include "usr_k2_lan.h"
#include "my_wifi.h"
#include <string.h>
#include "my_global_lib.h"
#include "transaction.h"

#if CONFIG_MCU_LCD_CONTROLLER_ILI9341
#include "esp_lcd_ili9341.h"
#endif

#if CONFIG_MCU_LCD_TOUCH_CONTROLLER_XPT2046
#include "esp_lcd_touch_xpt2046.h"
#endif

// static const char *TAG = "spi_lcd_touch_main";

void pulse_check_task(void *pvParameters);
void get_update_device(void *pvParameters);
void cleanup_device_info(void);

#define LCD_HOST SPI3_HOST
#define TOUCH_HOST SPI2_HOST

// #define MCU_LCD_PIXEL_CLOCK_HZ (20 * 1000 * 1000)
#define MCU_LCD_PIXEL_CLOCK_HZ (30 * 1000 * 1000)
#define MCU_LCD_BK_LIGHT_ON_LEVEL 1
#define MCU_LCD_BK_LIGHT_OFF_LEVEL !MCU_LCD_BK_LIGHT_ON_LEVEL
#define MCU_PIN_NUM_SCLK 14
#define MCU_PIN_NUM_MOSI 13
#define MCU_PIN_NUM_MISO 12
#define MCU_PIN_NUM_LCD_DC 2
#define MCU_PIN_NUM_LCD_RST 4
#define MCU_PIN_NUM_LCD_CS 15
#define MCU_PIN_NUM_BK_LIGHT 21
#define MCU_PIN_NUM_TOUCH_CS 33
#define MCU_PIN_NUM_TOUCH_MOSI 32
#define MCU_PIN_NUM_TOUCH_MISO 39
#define MCU_PIN_NUM_TOUCH_SCLK 25

#if CONFIG_MCU_LCD_CONTROLLER_ILI9341
#define MCU_LCD_H_RES 240
#define MCU_LCD_V_RES 320

#endif
#define MCU_LCD_CMD_BITS 8
#define MCU_LCD_PARAM_BITS 8

#define MCU_LVGL_DRAW_BUF_LINES 20
#define MCU_LVGL_TICK_PERIOD_MS 2
#define MCU_LVGL_TASK_MAX_DELAY_MS 500
#define MCU_LVGL_TASK_MIN_DELAY_MS 1
#define MCU_LVGL_TASK_STACK_SIZE (6 * 1024)
#define MCU_LVGL_TASK_PRIORITY 2

#define UART_PORT_MICROUSB UART_NUM_0
#define UART_PORT_LAN UART_NUM_2
#define UART_BAUD_RATE 115200
#define UART_BUF_SIZE 1024

extern void lvgl_ui_pre_startup(lv_disp_t *disp);

static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}

static void example_lvgl_port_update_callback(lv_display_t *disp)
{
    esp_lcd_panel_handle_t panel_handle = lv_display_get_user_data(disp);
    lv_display_rotation_t rotation = lv_display_get_rotation(disp);

    switch (rotation)
    {
    case LV_DISPLAY_ROTATION_0:
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, true, false);
        break;
    case LV_DISPLAY_ROTATION_90:
        esp_lcd_panel_swap_xy(panel_handle, true);
        esp_lcd_panel_mirror(panel_handle, true, true);
        break;
    case LV_DISPLAY_ROTATION_180:
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, false, true);
        break;
    case LV_DISPLAY_ROTATION_270:
        esp_lcd_panel_swap_xy(panel_handle, true);
        esp_lcd_panel_mirror(panel_handle, false, false);
        break;
    }
}

static void example_lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    example_lvgl_port_update_callback(disp);
    esp_lcd_panel_handle_t panel_handle = lv_display_get_user_data(disp);
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    lv_draw_sw_rgb565_swap(px_map, (offsetx2 + 1 - offsetx1) * (offsety2 + 1 - offsety1));
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);
}

#if CONFIG_MCU_LCD_TOUCH_ENABLED
static void example_lvgl_touch_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    uint16_t touchpad_x[1] = {0};
    uint16_t touchpad_y[1] = {0};
    uint8_t touchpad_cnt = 0;

    esp_lcd_touch_handle_t touch_pad = lv_indev_get_user_data(indev);
    esp_lcd_touch_read_data(touch_pad);
    bool touchpad_pressed = esp_lcd_touch_get_coordinates(touch_pad, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 1);

    if (touchpad_pressed && touchpad_cnt > 0)
    {
        data->point.x = touchpad_x[0];
        data->point.y = touchpad_y[0];
        data->state = LV_INDEV_STATE_PRESSED;
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
#endif

static void example_increase_lvgl_tick(void *arg)
{
    lv_tick_inc(MCU_LVGL_TICK_PERIOD_MS);
}

static void example_lvgl_port_task(void *arg)
{
    // ESP_LOGI(TAG, "Starting LVGL task");

    while (1)
    {
        uint32_t time_till_next_ms;
        lv_lock();
        time_till_next_ms = lv_timer_handler();
        lv_unlock();
        time_till_next_ms = MAX(time_till_next_ms, 10);
        vTaskDelay(pdMS_TO_TICKS(time_till_next_ms));
    }
}

void gpio_init_custom();
static void my_gpio_isr_handler(void *arg);
void uart_response(void *arg);
char *extract_between_std(const char *input);
void pulseOutputTask();
void save_info_task(void *pvParameters);

// Fungsi helper untuk update UI secara thread-safe
void update_remain_time_label(void *data);

#define GPIO_INPUT_PIN 34  // GPIO pin for pulse input pulse
#define GPIO_OUTPUT_PIN 26 // GPIO pin for pulse input pulse
#define ESP_INTR_FLAG_DEFAULT 0

#define PULSE_TIMEOUT_MS 500
#define BILL_ACCEPTOR_GPIO GPIO_NUM_4

static volatile int pulseCount = 0;
static volatile int timeoutInSecond = 0;
static volatile int isPreviewShow = 0;
static TimerHandle_t pulse_timer;

void save_info_task(void *pvParameters)
{
    transaction_params_t *params = (transaction_params_t *)pvParameters;

    // Use static buffer to reduce stack usage - increased size for safety
    static char query_add_transaction[160];
    int written = snprintf(query_add_transaction, sizeof(query_add_transaction),
                           "<ADDTRANSACTION,%s,%s,%d>",
                           params->device_id, params->unit_sn, params->gross_price);

    // Check for buffer overflow
    if (written >= sizeof(query_add_transaction))
    {
        DEBUG_PRINTLN("ERROR: Transaction query truncated!");
        free(params);
        vTaskDelete(NULL);
        return;
    }

    int32_t is_wifiorlan = 0;
    const char *nvs_namespace = "storage";
    load_nvs_i32(nvs_namespace, "is_wifiorlan", &is_wifiorlan);

    if (is_wifiorlan == NETWORK_LAN) // using LAN
    {
        send_at_command(query_add_transaction);
    }
    else if (is_wifiorlan == NETWORK_WIFI)
    { // using WiFi
        make_http_request(query_add_transaction);
    }
    else
    {
        DEBUG_PRINTLN("Network connection not configured (is_wifiorlan = %d)", (int)is_wifiorlan);
    }

    // Free allocated memory to prevent memory leak
    free(params);

// Check remaining stack before task deletion (only if DEBUG_MODE enabled)
#if DEBUG_MODE
    UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    DEBUG_PRINTLN("save_info_task stack remaining: %u bytes", (unsigned int)uxHighWaterMark);
#endif

    vTaskDelete(NULL);
}

void pulse_timeout_callback(TimerHandle_t xTimer)
{
    int32_t valueDuty;
    int32_t valueDuration;
    int32_t valuePulse;
    int32_t valueMoney;
    int32_t valueRateConversion;

    if (load_nvs_i32("storage", "duty", &valueDuty) != ESP_OK)
        valueDuty = 100;
    if (load_nvs_i32("storage", "duration", &valueDuration) != ESP_OK)
        valueDuration = 100;
    if (load_nvs_i32("storage", "pulse", &valuePulse) != ESP_OK)
        valuePulse = 0;
    if (load_nvs_i32("storage", "money", &valueMoney) != ESP_OK)
        valueMoney = 0;
    if (load_nvs_i32("storage", "RateConversion", &valueRateConversion) != ESP_OK)
        valueRateConversion = 0;

    int realPulse = pulseCount; // contoh 50
    int totalMoneyPulse = valueRateConversion * realPulse; // contoh 1000 x 50 = 50000
    int validUnits = totalMoneyPulse / valueMoney; // contoh 50000 / 20000 = 2.5
    int sisa = totalMoneyPulse % valueMoney; // contoh 50000 % 20000 = 10000

    if (validUnits > 0) // Ada minimal 1 unit uang valid
    {
        printf("Pulsa sebenarnya : %d\n", realPulse);
        // Proses bagian uang yang valid (kelipatan 10000)
        int decreasePulse = (validUnits * valueMoney) / valueRateConversion;
        printf("Dapat pulsa: %d, Pulsa dikurangi: %d\n", validUnits, decreasePulse);

        pulseCount -= decreasePulse; // contoh 50 - 40 = 10

        if(ui_Label11 != NULL) {
            char bufferSisa[16];
            snprintf(bufferSisa, sizeof(bufferSisa), "%d", sisa);
            lv_label_set_text(ui_Label11, bufferSisa); // valid
            printf("Tampilkan sisa : %s\n", bufferSisa);
        }

        printf("Pulsa : %ld * Unit valid: %d = %ld\n", valuePulse, validUnits, (long int)(valuePulse * validUnits));
        for (int i = 0; i < (valuePulse * validUnits); i++) // 1 x 2 = 2 pulse
        {
            gpio_set_level(GPIO_OUTPUT_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(valueDuty));
            gpio_set_level(GPIO_OUTPUT_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(valueDuration));
        }

        // Simpan statsPulse
        int32_t valueStatsHistory;
        if (load_nvs_i32("storage", "statsPulse", &valueStatsHistory) != ESP_OK)
            valueStatsHistory = 0;
        int32_t newvalueStatsHistory = valueStatsHistory + (valuePulse * validUnits);
        if (newvalueStatsHistory > 9999)
            save_nvs_i32("storage", "statsPulse", 0);
        else
            save_nvs_i32("storage", "statsPulse", newvalueStatsHistory);

        // Simpan statsMoney
        int32_t valueMoneyHistory;
        if (load_nvs_i32("storage", "statsMoney", &valueMoneyHistory) != ESP_OK)
            valueMoneyHistory = 0;
        int32_t newvalueMoneyHistory = valueMoneyHistory + (validUnits * valueMoney);
        if (newvalueMoneyHistory > 5000000)
        {
            newvalueMoneyHistory = newvalueMoneyHistory - valueMoneyHistory;
            save_nvs_i32("storage", "statsMoney", newvalueMoneyHistory);
        }
        else
        {
            save_nvs_i32("storage", "statsMoney", newvalueMoneyHistory);
        }

        transaction_params_t *params = malloc(sizeof(transaction_params_t));
        strcpy(params->device_id, CONFIG_MCU_DEVICEID);
        strcpy(params->unit_sn, DEVICE_INFO.UnitSn);
        params->gross_price = validUnits * valueMoney;
        xTaskCreate(&save_info_task, "save_info_task", 8192, params, 5, NULL);

        gpio_set_level(GPIO_OUTPUT_PIN, 0);

        // Kalau ada sisa, reset waktu kembali ke hitungan awal, tapi JANGAN tampilkan di label preview di sini
        if (sisa > 0)
        {
            printf("Kondisi sisa > 0\n");
            // Reset waktu kembali ke hitungan awal jika ada sisa
            timeoutInSecond = 0;
        } else {
            // pindah ke halaman sukses
            lv_async_call(go_page6, NULL);

            // Reset timeout dan preview
            timeoutInSecond = 0;
            isPreviewShow = 0;
            pulseCount = 0; // Reset pulse count

            // Reset label sisa ke 0 agar tidak tertambah di proses berikutnya
            if (ui_Label11 != NULL) {
                lv_label_set_text(ui_Label11, "0");
            }

            //DEBUG_PRINTLN("Kondisi sisa <= 0");
            printf("Kondisi sisa <= 0");
        }
    }
    else
    {
        printf("Tidak ada uang valid sama sekali, langsung tampilkan total\n");
        // Tidak ada uang valid sama sekali, langsung tampilkan total
        char bufferTotalMoney[16];
        snprintf(bufferTotalMoney, sizeof(bufferTotalMoney), "%d", totalMoneyPulse);
        lv_label_set_text(ui_Label11, bufferTotalMoney);
    }
}

static void IRAM_ATTR my_gpio_isr_handler(void *arg)
{
    // Unused parameter - suppressed to avoid compiler warning
    (void)arg;

    // Tambah counter pulsa (variabel volatile untuk keamanan thread)
    // Menghitung setiap transisi edge yang terdeteksi oleh bill acceptor
    pulseCount++;

    // Variabel untuk melacak apakah ada task prioritas tinggi yang terbangun oleh operasi ISR
    // Digunakan untuk switching konteks FreeRTOS yang tepat dari konteks interrupt
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Reset timer timeout pulsa untuk memulai ulang perhitungan mundur
    // Ini memperpanjang jendela pembayaran setiap kali pulsa baru terdeteksi
    // Timer akan expired setelah PULSE_TIMEOUT_MS jika tidak ada pulsa lagi
    xTimerResetFromISR(pulse_timer, &xHigherPriorityTaskWoken);

    // Jika operasi ISR membangunkan task prioritas tinggi,
    // serahkan kontrol ke task tersebut segera setelah keluar dari ISR
    // Ini memastikan perilaku real-time yang tepat dan penjadwalan task
    if (xHigherPriorityTaskWoken)
        portYIELD_FROM_ISR();
}

// Fungsi untuk inisialisasi GPIO custom untuk sistem pembayaran
void gpio_init_custom()
{
    /* Konfigurasi Input untuk Bill Acceptor (Penerima Uang Kertas) */
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE,           // Interrupt hanya pada rising edge (LOW ke HIGH) - menghindari double count
        .mode = GPIO_MODE_INPUT,                  // Mode input untuk menerima sinyal
        .pin_bit_mask = (1ULL << GPIO_INPUT_PIN), // Pin yang akan digunakan (GPIO 34)
        .pull_up_en = GPIO_PULLUP_DISABLE,        // Disable pull-up internal
        .pull_down_en = GPIO_PULLDOWN_DISABLE};   // Disable pull-down internal

    // Terapkan konfigurasi GPIO
    gpio_config(&io_conf);

    // Install service interrupt GPIO dengan flag default
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    // Tambahkan handler interrupt untuk pin GPIO_INPUT_PIN
    // Setiap kali ada perubahan pada pin ini, my_gpio_isr_handler akan dipanggil
    gpio_isr_handler_add(GPIO_INPUT_PIN, my_gpio_isr_handler, (void *)GPIO_INPUT_PIN);

    // DEBUG: Log bahwa GPIO interrupt sudah diinisialisasi
    // ESP_LOGI(TAG, "GPIO Interrupt initialized on GPIO%d", (int)GPIO_INPUT_PIN);

    /* Konfigurasi Output untuk Relay Coin (Output Koin) */
    gpio_config_t io_conf_out = {
        .pin_bit_mask = (1ULL << GPIO_OUTPUT_PIN), // Pin output (GPIO 26)
        .mode = GPIO_MODE_OUTPUT,                  // Mode output untuk mengirim sinyal
        .pull_up_en = GPIO_PULLUP_DISABLE,         // Disable pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE,     // Disable pull-down
        .intr_type = GPIO_INTR_DISABLE};           // Tidak perlu interrupt untuk output

    // Terapkan konfigurasi GPIO output
    gpio_config(&io_conf_out);

    // while (1) {
    //     // Tes OUTPUT: nyalakan relay 1 detik, matikan 1 detik
    //     gpio_set_level(GPIO_OUTPUT_PIN, 1);
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    //     gpio_set_level(GPIO_OUTPUT_PIN, 0);
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    // }
}

void uart_response(void *arg)
{
    uint8_t data[128];
    int pos = 0;

    while (1)
    {
        int len = uart_read_bytes(UART_PORT_MICROUSB, data + pos, sizeof(data) - pos - 1, pdMS_TO_TICKS(1000));
        if (len > 0)
        {
            pos += len;
            for (int i = 0; i < pos; i++)
            {
                if (data[i] == '\n')
                {
                    data[i] = '\0';
                    char *receiveData = (char *)data;
                    char *dataExtract = extract_between_std(receiveData);
                    char *copy = strdup(dataExtract);
                    char *token = strtok(copy, ",");

                    int index = 0;
                    int first_token = 0;
                    while (token != NULL)
                    {
                        int valueNumber = atoi(token);
                        switch (index)
                        {
                        case 0:
                            first_token = valueNumber;
                            break;
                        case 1:
                            if (first_token == 10001)
                                save_nvs_i32("storage", "money", valueNumber);
                            else if (first_token == 10002)
                                load_nvs_data();
                            else if (first_token == 10003)
                            {
                                // Untuk development
                                printf("<s>10003,%s<e>\n", CONFIG_MCU_DEVICEID);

                                // Untuk production, gunakan ID unik
                                // Berdasarkan MAC address atau chip ID
                                // Generate unique device ID based on MAC address
                                // uint8_t mac[6];
                                // esp_read_mac(mac, ESP_MAC_WIFI_STA);
                                // char unique_device_id[32];
                                // snprintf(unique_device_id, sizeof(unique_device_id),
                                //         "%02X%02X%02X%02X%02X%02X",
                                //         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                                // printf("<s>10003,%s<e>\n", unique_device_id);
                            }
                            else if (first_token == 10004)
                                xTaskCreate(pulseOutputTask, "PulseOutputTask", 2048, NULL, 5, NULL);
                            else if (first_token == 10005)
                            {
                                printf("<s>10005,%s<e>\n", "Resetting device...");
                                esp_restart();
                            }
                            else if (first_token == 99999)
                            {
                                save_nvs_i32("storage", "statsPulse", 0);
                                save_nvs_i32("storage", "statsMoney", 0);
                                printf("<s>99999,%s<e>\n", "Resetting pulse...");
                            }
                            else if (first_token == 77777)
                            {
                                char query_ping_server[32]; // Reduced size for simple <PING> command
                                snprintf(query_ping_server, sizeof(query_ping_server), "<PING>");

                                int32_t is_wifiorlan = 0;
                                const char *nvs_namespace = "storage";
                                load_nvs_i32(nvs_namespace, "is_wifiorlan", &is_wifiorlan);

                                if (is_wifiorlan == NETWORK_LAN) // using LAN
                                {
                                    send_at_command(query_ping_server);
                                }
                                else if (is_wifiorlan == NETWORK_WIFI)
                                { // using WiFi
                                    make_http_request(query_ping_server);
                                }
                                else
                                {
                                    DEBUG_PRINTLN("Network connection not configured (is_wifiorlan = %d)", (int)is_wifiorlan);
                                }
                            }
                            else if (first_token == 88888)
                            {
                                printf("<s>88888,%s<e>\n", "Restarting device...");
                                esp_restart();
                            }
                            else if (first_token == 66661)
                            {
                                printf("<s>66661,%s<e>\n", "Connecting to Wi-Fi...");
                                my_wifi_connect();
                                DEBUG_PRINTLN("Connect Wi-FI");
                            }
                            else if (first_token == 66662)
                            {
                                printf("<s>66662,%s<e>\n", "Disconnecting Wi-Fi...");
                                my_wifi_disconnect();
                                DEBUG_PRINTLN("Disconnect Wi-FI");
                            }
                            else if (first_token == 66663)
                            {
                                const char *nvs_namespace = "storage";
                                char server_host[30] = {0};
                                load_nvs_str(nvs_namespace, "server_host", server_host, sizeof(server_host));

                                // Kirim request - /endpoint akan ditambahkan otomatis di fungsi
                                char post_data[64];
                                snprintf(post_data, sizeof(post_data), "<GETINFO,%s>", CONFIG_MCU_DEVICEID);
                                char *response_wifi = my_wifi_send_getinfo_request(server_host, post_data);
                                if (response_wifi)
                                {
                                    DEBUG_PRINTLN("Response from server : %s", response_wifi);
                                    // Bebaskan memori yang dialokasikan untuk response
                                    free(response_wifi);
                                }
                                else
                                {
                                    DEBUG_PRINTLN("Request failed or no response.");
                                }
                                printf("<s>66663,%s<e>\n", "Request sent to server");
                            }
                            else if (first_token == 66664)
                            {
                                if (my_wifi_is_connected() == 1)
                                    DEBUG_PRINTLN("Wi-Fi is connected");
                                else
                                    DEBUG_PRINTLN("Wi-Fi is not connected");

                                printf("<s>66664,%s<e>\n", my_wifi_is_connected() == 1 ? "Connected" : "Not Connected");
                            }

                            break;
                        case 2:
                            if (first_token == 10001)
                                save_nvs_i32("storage", "pulse", valueNumber);
                            break;
                        case 3:
                            if (first_token == 10001)
                                save_nvs_i32("storage", "duty", valueNumber);
                            break;
                        case 4:
                            if (first_token == 10001)
                                save_nvs_i32("storage", "duration", valueNumber);
                            break;
                        case 5:
                            if (first_token == 10001)
                                save_nvs_i32("storage", "qrpulse", valueNumber);
                            break;
                        case 6:
                            if (first_token == 10001)
                                save_nvs_i32("storage", "RateConversion", valueNumber);
                            break;
                        case 7:
                            if (first_token == 10001)
                                save_nvs_i32("storage", "ExpiredTimeBill", valueNumber);
                            break;
                        case 8:
                            if (first_token == 10001)
                                save_nvs_str("storage", "wifi_ap", token);
                            break;
                        case 9:
                            if (first_token == 10001)
                                save_nvs_str("storage", "pass_ap", token);
                            break;
                        case 10:
                            if (first_token == 10001)
                                save_nvs_str("storage", "server_host", token);
                            break;
                        case 11:
                            if (first_token == 10001)
                                save_nvs_i32("storage", "is_wifiorlan", valueNumber);
                            break;
                        }
                        token = strtok(NULL, ",");
                        index++;
                    }
                    free(copy);
                    // Free dataExtract to prevent memory leak
                    if (dataExtract)
                    {
                        free(dataExtract);
                    }
                    memmove(data, data + i + 1, pos - i - 1);
                    pos -= (i + 1);
                    break;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void cleanup_device_info(void)
{
    // Reset all fields to empty strings instead of freeing pointers
    memset(&DEVICE_INFO, 0, sizeof(DEVICE_INFO));

    // Set safe defaults
    strncpy(DEVICE_INFO.IsActive, "1", sizeof(DEVICE_INFO.IsActive) - 1);
    strncpy(DEVICE_INFO.IsProduction, "1", sizeof(DEVICE_INFO.IsProduction) - 1);
    strncpy(DEVICE_INFO.UnitSn, "ESP32DEV", sizeof(DEVICE_INFO.UnitSn) - 1);
    strncpy(DEVICE_INFO.CurCode, "IDR", sizeof(DEVICE_INFO.CurCode) - 1);
    strncpy(DEVICE_INFO.GrossAmountTc, "10000", sizeof(DEVICE_INFO.GrossAmountTc) - 1);
    strncpy(DEVICE_INFO.Location, "DEFAULT", sizeof(DEVICE_INFO.Location) - 1);
    strncpy(DEVICE_INFO.Area, "DEFAULT", sizeof(DEVICE_INFO.Area) - 1);
    strncpy(DEVICE_INFO.OvoTid, "DEFAULT", sizeof(DEVICE_INFO.OvoTid) - 1);

    DEBUG_PRINTLN("DEVICE_INFO reset to safe defaults");
}

void app_main(void)
{
    init_nvs();
    usr_k2_init();

    esp_task_wdt_deinit();

    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    uart_driver_install(UART_PORT_MICROUSB, UART_BUF_SIZE, 0, 0, NULL, 0);
    uart_param_config(UART_PORT_MICROUSB, &uart_config);

    xTaskCreate(uart_response, "uart_response", 6144, NULL, 10, NULL);
    xTaskCreate(pulse_check_task, "Pulse Check Task", 2048, NULL, 5, NULL);

    gpio_init_custom();
    pulse_timer = xTimerCreate("pulse_timer", pdMS_TO_TICKS(PULSE_TIMEOUT_MS), pdFALSE, NULL, pulse_timeout_callback);

    // ESP_LOGI(TAG, "Turn off LCD backlight");
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << MCU_PIN_NUM_BK_LIGHT};
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    DEBUG_PRINTLN("Initialize SPI bus Display");
    spi_bus_config_t buscfg = {
        .sclk_io_num = MCU_PIN_NUM_SCLK,
        .mosi_io_num = MCU_PIN_NUM_MOSI,
        .miso_io_num = MCU_PIN_NUM_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = MCU_LCD_H_RES * 80 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, 0));

    DEBUG_PRINTLN("Initialize SPI bus Touch");
    spi_bus_config_t buscfg_touch = {
        .mosi_io_num = MCU_PIN_NUM_TOUCH_MOSI,
        .miso_io_num = MCU_PIN_NUM_TOUCH_MISO,
        .sclk_io_num = MCU_PIN_NUM_TOUCH_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = MCU_LCD_H_RES * 80 * sizeof(uint16_t),
    };

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg_touch, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK)
    {
        ESP_LOGE("SPI", "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return;
    }

    // ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = MCU_PIN_NUM_LCD_DC,
        .cs_gpio_num = MCU_PIN_NUM_LCD_CS,
        .pclk_hz = MCU_LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = MCU_LCD_CMD_BITS,
        .lcd_param_bits = MCU_LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = MCU_PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
#if CONFIG_MCU_LCD_CONTROLLER_ILI9341
    // ESP_LOGI(TAG, "Install ILI9341 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));
#endif

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    // ESP_LOGI(TAG, "Turn on LCD backlight");
    gpio_set_level(MCU_PIN_NUM_BK_LIGHT, MCU_LCD_BK_LIGHT_ON_LEVEL);

    // ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();

    lv_display_t *display = lv_display_create(MCU_LCD_H_RES, MCU_LCD_V_RES);
    lv_display_set_default(display);
    size_t draw_buffer_sz = MCU_LCD_H_RES * MCU_LVGL_DRAW_BUF_LINES * sizeof(lv_color16_t);

    void *buf1 = spi_bus_dma_memory_alloc(LCD_HOST, draw_buffer_sz, 0);
    assert(buf1);
    void *buf2 = spi_bus_dma_memory_alloc(TOUCH_HOST, draw_buffer_sz, 0);
    assert(buf2);
    lv_display_set_buffers(display, buf1, buf2, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_user_data(display, panel_handle);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(display, example_lvgl_flush_cb);
    lv_display_set_rotation(display, LV_DISPLAY_ROTATION_0);

    // ESP_LOGI(TAG, "Install LVGL tick timer");
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"};
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, MCU_LVGL_TICK_PERIOD_MS * 1000));

    // ESP_LOGI(TAG, "Register io panel event callback for LVGL flush ready notification");
    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = example_notify_lvgl_flush_ready,
    };
    ESP_ERROR_CHECK(esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, display));

#if CONFIG_MCU_LCD_TOUCH_ENABLED
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_spi_config_t tp_io_config =

#ifdef CONFIG_MCU_LCD_TOUCH_CONTROLLER_XPT2046
        ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(MCU_PIN_NUM_TOUCH_CS);
#endif

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)TOUCH_HOST, &tp_io_config, &tp_io_handle));

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = MCU_LCD_H_RES,
        .y_max = MCU_LCD_V_RES,
        .rst_gpio_num = -1,
        .int_gpio_num = -1,
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = CONFIG_MCU_LCD_MIRROR_Y,
        },
    };

    esp_lcd_touch_handle_t tp = NULL;

#if CONFIG_MCU_LCD_TOUCH_CONTROLLER_XPT2046
    // ESP_LOGI(TAG, "Initialize touch controller XPT2046");
    ESP_ERROR_CHECK(esp_lcd_touch_new_spi_xpt2046(tp_io_handle, &tp_cfg, &tp));
#endif

    static lv_indev_t *indev;
    indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_display(indev, display);
    lv_indev_set_user_data(indev, tp);
    lv_indev_set_read_cb(indev, example_lvgl_touch_cb);
#endif

    xTaskCreate(example_lvgl_port_task, "LVGL", MCU_LVGL_TASK_STACK_SIZE, NULL, MCU_LVGL_TASK_PRIORITY, NULL);

    lv_lock();
    ui_init();
    lv_unlock();

    /* Begin a task to get device updates */
    xTaskCreate(get_update_device, "get_update_device task", 8192, NULL, 10, NULL);
    /* End of task creation */

    char buffer[300];

    // Clean up any previously allocated memory first
    cleanup_device_info();

    esp_err_t err1 = load_nvs_str("storage", "IsActive", buffer, sizeof(buffer));
    if (err1 == ESP_OK && strlen(buffer) > 0 && strlen(buffer) < sizeof(DEVICE_INFO.IsActive))
    {
        strncpy(DEVICE_INFO.IsActive, buffer, sizeof(DEVICE_INFO.IsActive) - 1);
        DEVICE_INFO.IsActive[sizeof(DEVICE_INFO.IsActive) - 1] = '\0';
        DEBUG_PRINTLN("Loaded valid IsActive: %s", DEVICE_INFO.IsActive);
    }
    else
    {
        DEBUG_PRINTLN("WARNING: Failed to load IsActive from NVS, keeping default");
    }

    esp_err_t err2 = load_nvs_str("storage", "IsProduction", buffer, sizeof(buffer));
    if (err2 == ESP_OK && strlen(buffer) > 0 && strlen(buffer) < sizeof(DEVICE_INFO.IsProduction))
    {
        strncpy(DEVICE_INFO.IsProduction, buffer, sizeof(DEVICE_INFO.IsProduction) - 1);
        DEVICE_INFO.IsProduction[sizeof(DEVICE_INFO.IsProduction) - 1] = '\0';
        DEBUG_PRINTLN("Loaded valid IsProduction: %s", DEVICE_INFO.IsProduction);
    }
    else
    {
        DEBUG_PRINTLN("WARNING: Failed to load IsProduction from NVS, keeping default");
    }

    esp_err_t err3 = load_nvs_str("storage", "UnitSn", buffer, sizeof(buffer));
    if (err3 == ESP_OK && strlen(buffer) > 0 && strlen(buffer) < sizeof(DEVICE_INFO.UnitSn))
    {
        strncpy(DEVICE_INFO.UnitSn, buffer, sizeof(DEVICE_INFO.UnitSn) - 1);
        DEVICE_INFO.UnitSn[sizeof(DEVICE_INFO.UnitSn) - 1] = '\0';
        DEBUG_PRINTLN("Loaded valid UnitSn: %s", DEVICE_INFO.UnitSn);
    }
    else
    {
        DEBUG_PRINTLN("WARNING: Failed to load UnitSn from NVS, keeping default");
    }

    esp_err_t err4 = load_nvs_str("storage", "CurCode", buffer, sizeof(buffer));
    if (err4 == ESP_OK && strlen(buffer) > 0 && strlen(buffer) < sizeof(DEVICE_INFO.CurCode))
    {
        strncpy(DEVICE_INFO.CurCode, buffer, sizeof(DEVICE_INFO.CurCode) - 1);
        DEVICE_INFO.CurCode[sizeof(DEVICE_INFO.CurCode) - 1] = '\0';
        DEBUG_PRINTLN("Loaded valid CurCode: %s", DEVICE_INFO.CurCode);
    }
    else
    {
        DEBUG_PRINTLN("WARNING: Failed to load CurCode from NVS, keeping default");
    }

    esp_err_t err5 = load_nvs_str("storage", "GrossAmountTc", buffer, sizeof(buffer));
    if (err5 == ESP_OK && strlen(buffer) > 0 && strlen(buffer) < sizeof(DEVICE_INFO.GrossAmountTc))
    {
        // Validate that GrossAmountTc contains only valid characters
        bool valid = true;
        for (int i = 0; i < strlen(buffer); i++)
        {
            char c = buffer[i];
            if (!(c >= '0' && c <= '9') && c != '.' && c != ',')
            {
                valid = false;
                break;
            }
        }
        if (valid)
        {
            strncpy(DEVICE_INFO.GrossAmountTc, buffer, sizeof(DEVICE_INFO.GrossAmountTc) - 1);
            DEVICE_INFO.GrossAmountTc[sizeof(DEVICE_INFO.GrossAmountTc) - 1] = '\0';
            DEBUG_PRINTLN("Loaded valid GrossAmountTc: %s", DEVICE_INFO.GrossAmountTc);
        }
        else
        {
            DEBUG_PRINTLN("WARNING: Invalid GrossAmountTc in NVS, keeping default");
        }
    }
    else
    {
        DEBUG_PRINTLN("WARNING: Failed to load GrossAmountTc from NVS, keeping default");
    }

    esp_err_t err6 = load_nvs_str("storage", "Location", buffer, sizeof(buffer));
    if (err6 == ESP_OK && strlen(buffer) > 0 && strlen(buffer) < sizeof(DEVICE_INFO.Location))
    {
        strncpy(DEVICE_INFO.Location, buffer, sizeof(DEVICE_INFO.Location) - 1);
        DEVICE_INFO.Location[sizeof(DEVICE_INFO.Location) - 1] = '\0';
        DEBUG_PRINTLN("Loaded valid Location: %s", DEVICE_INFO.Location);
    }
    else
    {
        DEBUG_PRINTLN("WARNING: Failed to load Location from NVS, keeping default");
    }

    esp_err_t err7 = load_nvs_str("storage", "Area", buffer, sizeof(buffer));
    if (err7 == ESP_OK && strlen(buffer) > 0 && strlen(buffer) < sizeof(DEVICE_INFO.Area))
    {
        strncpy(DEVICE_INFO.Area, buffer, sizeof(DEVICE_INFO.Area) - 1);
        DEVICE_INFO.Area[sizeof(DEVICE_INFO.Area) - 1] = '\0';
        DEBUG_PRINTLN("Loaded valid Area: %s", DEVICE_INFO.Area);
    }
    else
    {
        DEBUG_PRINTLN("WARNING: Failed to load Area from NVS, keeping default");
    }

    esp_err_t err8 = load_nvs_str("storage", "OvoTid", buffer, sizeof(buffer));
    if (err8 == ESP_OK && strlen(buffer) > 0 && strlen(buffer) < sizeof(DEVICE_INFO.OvoTid))
    {
        strncpy(DEVICE_INFO.OvoTid, buffer, sizeof(DEVICE_INFO.OvoTid) - 1);
        DEVICE_INFO.OvoTid[sizeof(DEVICE_INFO.OvoTid) - 1] = '\0';
        DEBUG_PRINTLN("Loaded valid OvoTid: %s", DEVICE_INFO.OvoTid);
    }
    else
    {
        DEBUG_PRINTLN("WARNING: Failed to load OvoTid from NVS, keeping default");
    }

    const char *nvs_namespace = "storage";

    char wifi_ap[15] = {0};
    char pass_ap[15] = {0};
    int32_t is_wifiorlan = 0;

    load_nvs_str(nvs_namespace, "wifi_ap", wifi_ap, sizeof(wifi_ap));
    load_nvs_str(nvs_namespace, "pass_ap", pass_ap, sizeof(pass_ap));
    load_nvs_i32(nvs_namespace, "is_wifiorlan", &is_wifiorlan);

    if (is_wifiorlan == NETWORK_LAN)
    {
        DEBUG_PRINTLN("Using Internet LAN");
        // using LAN connection
    }
    else if (is_wifiorlan == NETWORK_WIFI)
    {
        if (wifi_ap[0] && pass_ap[0])
        {
            DEBUG_PRINTLN("Using Internet WiFi");
            my_wifi_init(wifi_ap, pass_ap);
        }
        else
        {
            DEBUG_PRINTLN("WiFi credentials not available");
        }
    }
    else
    {
        DEBUG_PRINTLN("Network connection not configured (is_wifiorlan = %d)", (int)is_wifiorlan);
    }
}

char *extract_between_std(const char *input)
{
    const char *start = strstr(input, "<s>");
    const char *end = strstr(input, "<e>");
    if (!start || !end || start > end)
        return NULL;
    start += 3;
    size_t len = end - start;
    char *result = (char *)malloc(len + 1);
    if (!result)
        return NULL;
    strncpy(result, start, len);
    result[len] = '\0';
    return result;
}

void pulseOutputTask()
{
    int32_t valueDuty;
    int32_t valueDuration;
    int32_t valuePulse;

    if (load_nvs_i32("storage", "duty", &valueDuty) != ESP_OK)
        valueDuty = 100;
    if (load_nvs_i32("storage", "duration", &valueDuration) != ESP_OK)
        valueDuration = 100;
    if (load_nvs_i32("storage", "pulse", &valuePulse) != ESP_OK)
        valuePulse = 0;

    for (int i = 0; i < (valuePulse); i++)
    {
        gpio_set_level(GPIO_OUTPUT_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(valueDuty));
        gpio_set_level(GPIO_OUTPUT_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(valueDuration));
    }
    vTaskDelete(NULL);
}

void pulse_check_task(void *pvParameters)
{
    while (1)
    {
        int32_t valueExpiredTimeBill;
        if (load_nvs_i32("storage", "ExpiredTimeBill", &valueExpiredTimeBill) != ESP_OK)
            valueExpiredTimeBill = 1;
        if (valueExpiredTimeBill < 1)
            valueExpiredTimeBill = 1;
        if (pulseCount > 0)
        {
            int remainTime = valueExpiredTimeBill - timeoutInSecond;
            
            // Pastikan remainTime tidak negatif untuk menghindari tampilan yang aneh
            if (remainTime < 0) {
                remainTime = 0;
            }
            
            char bufferRemainTime[16];
            snprintf(bufferRemainTime, sizeof(bufferRemainTime), "%d", remainTime);
            
            // Gunakan lv_async_call untuk thread-safe UI update
            // Alokasi memori dinamis untuk data yang akan dikirim ke LVGL task
            char *time_data = malloc(16);
            if (time_data != NULL) {
                strncpy(time_data, bufferRemainTime, 15);
                time_data[15] = '\0';
                
                // Update UI secara thread-safe menggunakan async call
                lv_async_call(update_remain_time_label, time_data);
            } else {
                DEBUG_PRINTLN("ERROR: Failed to allocate memory for UI update");
            }

            if (isPreviewShow == 0)
            {
                isPreviewShow = 1;
                // halaman preview pulsa dan waktu timeout
                lv_async_call(go_page9, NULL);
            }
            if (timeoutInSecond > valueExpiredTimeBill)
            {
                // Hentikan timer pulse_timer
                if (pulse_timer != NULL)
                {
                    xTimerStop(pulse_timer, 0);
                }

                // Hitung sisa uang terakhir dan tampilkan di label preview
                int32_t valueDuty, valueDuration, valuePulse, valueMoney, valueRateConversion;
                if (load_nvs_i32("storage", "duty", &valueDuty) != ESP_OK) valueDuty = 100;
                if (load_nvs_i32("storage", "duration", &valueDuration) != ESP_OK) valueDuration = 100;
                if (load_nvs_i32("storage", "pulse", &valuePulse) != ESP_OK) valuePulse = 0;
                if (load_nvs_i32("storage", "money", &valueMoney) != ESP_OK) valueMoney = 0;
                if (load_nvs_i32("storage", "RateConversion", &valueRateConversion) != ESP_OK) valueRateConversion = 0;
                int realPulse = pulseCount;
                int totalMoneyPulse = valueRateConversion * realPulse;
                int sisa = totalMoneyPulse % valueMoney;
                if (sisa > 0 && ui_Label11 != NULL) {
                    char bufferSisa[16];
                    snprintf(bufferSisa, sizeof(bufferSisa), "%d", sisa);
                    lv_label_set_text(ui_Label11, bufferSisa);
                }

                // ESP_LOGI(TAG, "Time is UP!");

                // Reset all if time is up
                pulseCount = 0;
                timeoutInSecond = 0;
                isPreviewShow = 0;

                // go to page failed
                lv_async_call(go_page4, NULL);
            }
            timeoutInSecond++;
        }
        else
        {
            isPreviewShow = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL);
}

void get_update_device(void *pvParameters)
{
    // Use static buffer to reduce stack usage
    static char query_req_device[50];

    snprintf(query_req_device, sizeof(query_req_device), "<GETINFO,%s>", CONFIG_MCU_DEVICEID);

    int32_t is_wifiorlan = 0;
    const char *nvs_namespace = "storage";
    load_nvs_i32(nvs_namespace, "is_wifiorlan", &is_wifiorlan);

    if (is_wifiorlan == NETWORK_LAN) // using LAN
    {
        send_at_command(query_req_device);
    }
    else if (is_wifiorlan == NETWORK_WIFI)
    { // using WiFi
        make_http_request(query_req_device);
    }
    else
    {
        DEBUG_PRINTLN("Network connection not configured (is_wifiorlan = %d)", (int)is_wifiorlan);
    }

// Check remaining stack before task deletion (only if DEBUG_MODE enabled)
#if DEBUG_MODE
    UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    DEBUG_PRINTLN("get_update_device stack remaining: %u bytes", (unsigned int)uxHighWaterMark);
#endif

    vTaskDelete(NULL);
}

// Implementasi fungsi helper untuk update UI secara thread-safe
void update_remain_time_label(void *data)
{
    char *time_str = (char *)data;
    if (time_str != NULL) {
        if (ui_Label10 != NULL) {
            // Fungsi ini akan dipanggil dari LVGL task yang sudah memiliki proper locking
            lv_label_set_text(ui_Label10, time_str);
        }
        // Bebaskan memori yang dialokasikan di pulse_check_task
        free(time_str);
    }
}