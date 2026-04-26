#include "cart_screen.h"
#include <stdio.h>
#include <string.h>

lv_obj_t *scr2;

static lv_obj_t *cart_list = NULL;
static const float prices[5] = {3, 7, 6, 5, 4};

extern void create_new_record(void);
extern void update_total_sum(void);
extern lv_obj_t *scr1;

static lv_obj_t* create_box_label(lv_obj_t *parent, const char *text, lv_color_t color, int x, int y)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_SIZE_CONTENT, 30);
    lv_obj_set_pos(cont,x,y);
    lv_obj_set_style_bg_color(cont,color,0);
    lv_obj_set_style_radius(cont,6,0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lab = lv_label_create(cont);
    lv_label_set_text(lab,text);
    lv_label_set_long_mode(lab, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(lab, LV_SIZE_CONTENT);
    lv_obj_clear_flag(lab, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(lab);

    return lab;
}

static void clear_cart_list(void)
{
    if (cart_list == NULL) return;
    lv_obj_clean(cart_list);
}

void refresh_cart_list(void)
{
    clear_cart_list();
    const char *names[5] = {"milk","chips","bread","apple","water"};
    int y = 10;

    create_box_label(cart_list, "Product", lv_palette_main(LV_PALETTE_BLUE), 10, y);
    create_box_label(cart_list, "Qty", lv_palette_main(LV_PALETTE_ORANGE), 110, y);
    create_box_label(cart_list, "Total", lv_palette_main(LV_PALETTE_RED), 210, y);
    y += 50;

    for (int i = 0; i < 5; i++)
    {
        if (cart_qty[i] <= 0.0f) continue;

        char qbuf[32], tbuf[32];
        sprintf(qbuf, "%.2f", cart_qty[i]);
        sprintf(tbuf, "%.2f", cart_qty[i] * prices[i]);

        create_box_label(cart_list, names[i], lv_palette_lighten(LV_PALETTE_BLUE,2), 10, y);
        create_box_label(cart_list, qbuf, lv_palette_lighten(LV_PALETTE_ORANGE,2), 110, y);
        create_box_label(cart_list, tbuf, lv_palette_lighten(LV_PALETTE_RED,2), 210, y);

        y += 50;
    }
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

void create_scr2(void)
{
    scr2 = lv_obj_create(NULL);

    lv_obj_t *btn_cont = lv_obj_create(scr2);
    lv_obj_set_size(btn_cont, 100, 300);
    lv_obj_align(btn_cont, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_set_style_bg_opa(btn_cont, 0, 0);

    lv_obj_t *btn_back = lv_btn_create(btn_cont);
    lv_obj_set_size(btn_back, 90, 40);
    lv_obj_align(btn_back, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, "Back");
    lv_obj_center(lbl_back);
    lv_obj_add_event_cb(btn_back, switch_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_check = lv_btn_create(btn_cont);
    lv_obj_set_size(btn_check, 90, 40);
    lv_obj_align(btn_check, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_t *lbl_check = lv_label_create(btn_check);
    lv_label_set_text(lbl_check, "Pay");
    lv_obj_center(lbl_check);
    lv_obj_add_event_cb(btn_check, show_checkout_popup, LV_EVENT_CLICKED, NULL);

    cart_list = lv_obj_create(scr2);
    lv_obj_set_size(cart_list, lv_disp_get_hor_res(NULL) - 120, lv_disp_get_ver_res(NULL) - 20);
    lv_obj_align(cart_list, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_scroll_dir(cart_list, LV_DIR_VER);
    lv_obj_set_style_pad_all(cart_list, 10, 0);
}

static void checkout_msgbox_cb(lv_event_t *e)
{
    lv_obj_t *mbox = lv_event_get_current_target(e);
    uint32_t btn_idx = lv_msgbox_get_active_btn(mbox);

    if (btn_idx == 0) {
        create_new_record();
    }

    lv_msgbox_close(mbox);
}

void show_checkout_popup(lv_event_t *e)
{
    float total_all = 0;
    for (int i = 0; i < 5; i++) total_all += cart_qty[i] * prices[i];

    char buf[128];
    snprintf(buf, sizeof(buf), "Total: %.2f", total_all);
    static const char *btns[] = {"Confirm", "Cancel", NULL};

    lv_obj_t *mbox = lv_msgbox_create(NULL, "Checkout", buf, btns, true);
    lv_obj_center(mbox);
    lv_obj_add_event_cb(mbox, checkout_msgbox_cb, LV_EVENT_VALUE_CHANGED, NULL);
}
