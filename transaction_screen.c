#include "transaction_screen.h"
#include <stdio.h>
#include <string.h>

lv_obj_t *scr3;

#define MAX_RECORDS 50

typedef struct {
    char items[256];
    float total;
} Record;

static Record record_list[MAX_RECORDS];
static int record_count = 0;
static lv_obj_t *record_container = NULL;

extern float cart_qty[5];
extern void update_total_sum(void);
extern lv_obj_t *scr1;

static const float prices[5] = {3, 7, 6, 5, 4};

static void refresh_records(void)
{
    if (!record_container) return;

    lv_obj_clean(record_container);
    lv_obj_set_flex_flow(record_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(record_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(record_container, 10, 0);
    lv_obj_set_style_pad_all(record_container, 5, 0);

    for (int i = 0; i < record_count; i++) {
        lv_obj_t *card = lv_obj_create(record_container);
        lv_obj_set_width(card, lv_disp_get_hor_res(NULL) - 40);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_border_color(card, lv_color_black(), 0);
        lv_obj_set_style_radius(card, 5, 0);
        lv_obj_set_style_pad_all(card, 8, 0);

        lv_obj_t *lbl_item = lv_label_create(card);
        lv_label_set_text_fmt(lbl_item, "Items: %s", record_list[i].items);
        lv_label_set_long_mode(lbl_item, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(lbl_item, lv_obj_get_width(card) - 16);

        lv_obj_t *lbl_total = lv_label_create(card);
        lv_label_set_text_fmt(lbl_total, "Total: %.2f", record_list[i].total);
    }
}

void create_new_record(void)
{
    if (record_count >= MAX_RECORDS) return;

    const char *item_names[5] = {"milk", "chips", "bread", "apple", "water"};
    char items_buf[256] = {0};
    float total_all = 0.0f;

    size_t buf_remaining = sizeof(items_buf);
    char *buf_ptr = items_buf;

    for (int i = 0; i < 5; i++) {
        if (cart_qty[i] <= 0.001f) continue;

        total_all += cart_qty[i] * prices[i];

        int written = snprintf(buf_ptr, buf_remaining, "%s:%.2f  ", item_names[i], cart_qty[i]);
        if (written < 0 || (size_t)written >= buf_remaining) {
            buf_ptr[buf_remaining - 1] = '\0';
            break;
        }
        buf_ptr += written;
        buf_remaining -= written;
    }

    if (strlen(items_buf) == 0) return;

    for (int i = record_count; i > 0; i--) {
        record_list[i] = record_list[i - 1];
    }

    record_list[0].total = total_all;
    strncpy(record_list[0].items, items_buf, sizeof(record_list[0].items) - 1);
    record_list[0].items[sizeof(record_list[0].items) - 1] = '\0';
    record_count++;

    memset(cart_qty, 0, sizeof(cart_qty));
    update_total_sum();

    if (record_container) refresh_records();
}

static void switch_btn_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    lv_obj_t *btn = lv_event_get_target(e);
    if(btn == NULL) return;
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    if(label == NULL) return;
    const char *txt = lv_label_get_text(label);
    if(txt == NULL) return;

    if(strcmp(txt, "Back") == 0)
    {
        lv_scr_load(scr1);
    }
}

void create_scr3_records(void)
{
    scr3 = lv_obj_create(NULL);

    lv_obj_t *btn_back = lv_btn_create(scr3);
    lv_obj_set_size(btn_back, 90, 40);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, "Back");
    lv_obj_center(lbl_back);
    lv_obj_add_event_cb(btn_back, switch_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *title = lv_label_create(scr3);
    lv_label_set_text(title, "Transaction Records");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    record_container = lv_obj_create(scr3);
    lv_obj_set_size(record_container,
                    lv_disp_get_hor_res(NULL) - 20,
                    lv_disp_get_ver_res(NULL) - 70);
    lv_obj_align(record_container, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_scroll_dir(record_container, LV_DIR_VER);
    lv_obj_set_style_pad_all(record_container, 10, 0);

    refresh_records();
}

void create_scr3(void)
{
    scr3 = lv_obj_create(NULL);

    lv_obj_t *lbl = lv_label_create(scr3);
    lv_label_set_text(lbl,"Records Page");
    lv_obj_align(lbl, LV_ALIGN_CENTER,0,0);

    lv_obj_t *btn_back = lv_btn_create(scr3);
    lv_obj_set_size(btn_back, 90, 40);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 10, 10);

    lv_obj_t *lbl_btn = lv_label_create(btn_back);
    lv_label_set_text(lbl_btn,"Back");
    lv_obj_center(lbl_btn);

    lv_obj_add_event_cb(btn_back, switch_btn_event_cb, LV_EVENT_CLICKED, NULL);
}
