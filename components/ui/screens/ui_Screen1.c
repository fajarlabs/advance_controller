#include "../ui.h"
#include "../usr_k2_lan.h"
#include "../safe_page.h"
#include "../my_global_lib.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lvgl.h"
#include "string.h"
#include <stdio.h>

//static const char *TAG = "ui_Screen1";

void system_ready(lv_timer_t *timer);

void screen1_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_SCREEN_LOADED)
    {
        // Ini dipanggil setelah screen tampil
        DEBUG_PRINTLN("Screen1 loaded!");
    }
}

void ui_Screen1_screen_init(void)
{
    ui_Screen1 = lv_obj_create(NULL);
    lv_obj_remove_flag(ui_Screen1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_Screen1, screen1_event_handler, LV_EVENT_ALL, NULL);

    ui_Image9 = lv_image_create(ui_Screen1);
    lv_image_set_src(ui_Image9, &ui_img_img1_png);
    lv_obj_set_width(ui_Image9, 240);
    lv_obj_set_height(ui_Image9, 320);
    lv_obj_set_align(ui_Image9, LV_ALIGN_CENTER);
    lv_obj_set_flex_flow(ui_Image9, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ui_Image9, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_add_flag(ui_Image9, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(ui_Image9, LV_OBJ_FLAG_SCROLLABLE);

    lv_timer_create(system_ready, 2000, NULL);
}

void system_ready(lv_timer_t *timer)
{
    DEBUG_PRINTLN("System is ready!");
    lv_async_call(go_page2, NULL);
    lv_timer_del(timer); // Hentikan timer setelah sukses
}