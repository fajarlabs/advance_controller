#include "ui.h"
#include <stdio.h>

void go_page1(void *data) {
    lv_lock();
    lv_scr_load(ui_Screen1);
    lv_unlock();
}

void go_page2(void *data) {
    lv_lock();
    lv_scr_load(ui_Screen2);
    lv_unlock();
}

void go_page3(void *data) {
    lv_lock();
    lv_scr_load(ui_Screen3);
    lv_unlock();
}

void go_page4(void *data) {
    lv_lock();
    lv_scr_load(ui_Screen4);
    lv_unlock();
}

void go_page5(void *data) {
    lv_lock();
    lv_scr_load(ui_Screen5);
    lv_unlock();
}

void go_page6(void *data) {
    printf("DEBUG: go_page6 called\n");
    lv_lock();
    if (ui_Screen6 != NULL) {
        printf("DEBUG: ui_Screen6 is valid, loading screen\n");
        lv_scr_load(ui_Screen6);
        printf("DEBUG: Screen6 loaded successfully\n");
    } else {
        printf("ERROR: ui_Screen6 is NULL!\n");
    }
    lv_unlock();
    printf("DEBUG: go_page6 completed\n");
}

void go_page7(void *data) {
    lv_lock();
    lv_scr_load(ui_Screen7);
    lv_unlock();
}

void go_page8(void *data) {
    lv_lock();
    lv_scr_load(ui_Screen8);
    lv_unlock();
}

void go_page9(void *data) {
    lv_lock();
    lv_scr_load(ui_Screen9);
    lv_unlock();
}