#include "event.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "read_file_to_array.h"
#include "sdram_malloc.h"

/* ===================== Screens ===================== */
lv_obj_t *scr1;
lv_obj_t *scr2;
lv_obj_t *scr3;

/* Keyboard + textarea */
static lv_obj_t *kb;
static lv_obj_t *ta;

/* Labels */
static lv_obj_t *qty_label[5];
static lv_obj_t *total_label[5];
static lv_obj_t *total_sum_label;


static lv_obj_t *img_list[5];   // ? ??????

/* Current index */
static int current_index = -1;

/* Prices */
static const float prices[5] = {3, 7, 6, 5, 4};

/* Decimal flags */
static const int is_decimal[5] = {0, 1, 1, 1, 0};

/*Quantity in cart*/
static float cart_qty[5] = {0.0f};

/*Strolling list*/
static lv_obj_t *cart_list = NULL;

/* ===================== 交易记录数据结构 ===================== */
#define MAX_RECORDS 50

typedef struct {
    char items[256];    // 商品名称+数量
    float total;        // 总价
} Record;

static Record record_list[MAX_RECORDS]; // 存储交易记录数组
static int record_count = 0;            // 当前记录条数

/* ===================== LVGL 容器 ===================== */
static lv_obj_t *record_container = NULL; // 显示交易记录的滚动容器

/* ===================== ????? ===================== */
static const char * kb_map_int[] = {
    "1","2","3","\n",
    "4","5","6","\n",
    "7","8","9","\n",
    "0","DEL","OK",""
};

static const char * kb_map_float[] = {
    "1","2","3","\n",
    "4","5","6","\n",
    "7","8","9","\n",
    ".","0","DEL","OK",""
};

/* ===================== ???? ===================== */
static bool is_point_in_obj(lv_obj_t *obj, lv_point_t *p)
{
    lv_area_t area;
    lv_obj_get_coords(obj, &area);

    return (p->x >= area.x1 && p->x <= area.x2 &&
            p->y >= area.y1 && p->y <= area.y2);
}

/* ===================== ???? ===================== */
static void sanitize_input(char *buf)
{
    int len = strlen(buf);
    int dot_count = 0;
    char new_buf[32] = {0};
    int j = 0;

    for(int i = 0; i < len; i++)
    {
        char c = buf[i];

        /* ????????? */
        if((c >= '0' && c <= '9') || c == '.')
        {
            /* ??????????? */
            if(is_decimal[current_index] == 0 && c == '.')
                continue;

            /* ??????? */
            if(c == '.')
            {
                dot_count++;
                if(dot_count > 1) continue;
            }

            /* ? ??????? */
if(j < sizeof(new_buf) - 1)
{
    new_buf[j] = c;
    j++;
}
        }
    }

    new_buf[j] = '\0';

    /* ? ?? ".5" ? "0.5" */
    if(new_buf[0] == '.')
    {
        memmove(new_buf+1, new_buf, strlen(new_buf)+1);
        new_buf[0] = '0';
    }

    /* ? ??????(??2?) */
char *dot = strchr(new_buf, '.');
if(dot)
{
    if(strlen(dot) > 3)
       if(is_decimal[current_index])
{
    float v = atof(new_buf);
    sprintf(new_buf, "%.2f", v);
}
}

    /* ? ?????(????) */
    float val = atof(new_buf);
    if(val > 99.99f)
    {
        strcpy(new_buf, "99.99");
    }

    strcpy(buf,new_buf);
}

static void update_total_sum(void)
{
    float sum = 0;

    for(int i = 0; i < 5; i++)
    {
        const char *txt = lv_label_get_text(total_label[i]);

        float val = 0;
        sscanf(txt, "Total:%f", &val);

        sum += val;
    }

    char buf[32];
    sprintf(buf, "SUM: %.2f", sum);

    lv_label_set_text(total_sum_label, buf);
}

/* ===================== ?????? ===================== */
static void clear_img_selection_anim(void)
{
    for(int i = 0; i < 5; i++)
    {
        if(img_list[i] == NULL) continue;

        /* ===== 1. ?????? ===== */
        lv_anim_t a1;
        lv_anim_init(&a1);
        lv_anim_set_var(&a1, img_list[i]);
        lv_anim_set_values(&a1, LV_OPA_20, LV_OPA_TRANSP);
        lv_anim_set_time(&a1, 150);
        lv_anim_set_exec_cb(&a1, (lv_anim_exec_xcb_t)lv_obj_set_style_bg_opa);
        lv_anim_start(&a1);

        /* ===== 2. ??????(??)===== */
        lv_obj_set_style_border_width(img_list[i], 0, 0);
        lv_obj_set_style_border_color(img_list[i], lv_color_black(), 0);

        /* ===== 3. ???? ===== */
        lv_obj_set_style_radius(img_list[i], 0, 0);
    }
}

static void blank_click_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_PRESSED) return;

    if(lv_obj_has_flag(kb, LV_OBJ_FLAG_HIDDEN)) return;

    lv_point_t p;
    lv_indev_get_point(lv_indev_get_act(), &p);

    if(is_point_in_obj(kb, &p)) return;
    if(is_point_in_obj(ta, &p)) return;

    /* ===== 1. ???? ===== */
    lv_textarea_set_text(ta, "");

    /* ===== 2. ???? ===== */
    lv_anim_t a_kb;
    lv_anim_init(&a_kb);
    lv_anim_set_var(&a_kb, kb);
    lv_anim_set_values(&a_kb, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a_kb, 200);
    lv_anim_set_exec_cb(&a_kb, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_start(&a_kb);

    /* ===== 3. ????? ===== */
    lv_anim_t a_ta;
    lv_anim_init(&a_ta);
    lv_anim_set_var(&a_ta, ta);
    lv_anim_set_values(&a_ta, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a_ta, 200);
    lv_anim_set_exec_cb(&a_ta, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_start(&a_ta);

    /* ===== 4. ????(????) ===== */
    clear_img_selection_anim();

    /* ===== 5. ?? ===== */
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ta, LV_OBJ_FLAG_HIDDEN);

    lv_obj_set_style_opa(kb, LV_OPA_COVER, 0);
    lv_obj_set_style_opa(ta, LV_OPA_COVER, 0);

    /* ===== 6. ???? ===== */
    current_index = -1;
}

/* ===================== ?? ===================== */
static lv_obj_t* create_box_label(lv_obj_t *parent, const char *text, lv_color_t color, int x, int y)
{
    lv_obj_t *cont = lv_obj_create(parent);

    /* ? ???:???????? */
    lv_obj_set_size(cont, LV_SIZE_CONTENT, 30);

    lv_obj_set_pos(cont,x,y);
    lv_obj_set_style_bg_color(cont,color,0);
    lv_obj_set_style_radius(cont,6,0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lab = lv_label_create(cont);
    lv_label_set_text(lab,text);

    /* ? ???? */
    lv_label_set_long_mode(lab, LV_LABEL_LONG_CLIP);

    /* ? ???:?label????? */
    lv_obj_set_width(lab, LV_SIZE_CONTENT);

    lv_obj_clear_flag(lab, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_center(lab);

    return lab;
}

static void clear_cart_list(void)
{
    if (cart_list == NULL) return;
    lv_obj_clean(cart_list); // ???????
}

// ?????:????????
void refresh_cart_list(void)
{
    clear_cart_list();
    const char *names[5] = {"milk","chips","bread","apple","water"};
    int y = 10; // ??Y??

    // ???
    create_box_label(cart_list, "Product", lv_palette_main(LV_PALETTE_BLUE), 10, y);
    create_box_label(cart_list, "Qty", lv_palette_main(LV_PALETTE_ORANGE), 110, y);
    create_box_label(cart_list, "Total", lv_palette_main(LV_PALETTE_RED), 210, y);
    y += 50;

    // ??5???,?????>0?
    for (int i = 0; i < 5; i++)
    {
        if (cart_qty[i] <= 0.0f) continue;

        char qbuf[32], tbuf[32];
        sprintf(qbuf, "%.2f", cart_qty[i]);
        sprintf(tbuf, "%.2f", cart_qty[i] * prices[i]);

        // ????:????????
        create_box_label(cart_list, names[i], lv_palette_lighten(LV_PALETTE_BLUE,2), 10, y);
        create_box_label(cart_list, qbuf, lv_palette_lighten(LV_PALETTE_ORANGE,2), 110, y);
        create_box_label(cart_list, tbuf, lv_palette_lighten(LV_PALETTE_RED,2), 210, y);

        y += 50; // ????,??Y?? ? ????
    }
}

/* ===================== ???? ===================== */
void switch_btn_event_cb(lv_event_t *e)
{
    /* 1?? ??????? */
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    /* 2?? ?????? */
    lv_obj_t *btn = lv_event_get_target(e);
    if(btn == NULL) return;

    /* 3?? ?????? label(?????) */
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    if(label == NULL) return;

    const char *txt = lv_label_get_text(label);
    if(txt == NULL) return;

    /* 4?? ???? */
    if(strcmp(txt, "Cart") == 0)
    {
        lv_scr_load(scr2);
        refresh_cart_list();   // ??????
    }
    else if(strcmp(txt, "Records") == 0)
    {
        lv_scr_load(scr3);
    }
    else if(strcmp(txt, "Back") == 0)
    {
        lv_scr_load(scr1);
    }
}

static void img_click_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    int idx = (int)lv_event_get_user_data(e);
    if(idx < 0 || idx >= 5) return;

    current_index = idx;

    /* ===== 1. ????? ===== */
    clear_img_selection_anim();

    /* ===== 2. ????? ===== */
    if(img_list[idx])
    {
        lv_obj_set_style_border_width(img_list[idx], 3, 0);
        lv_obj_set_style_border_color(img_list[idx],
            lv_palette_main(LV_PALETTE_RED), 0);

        lv_obj_set_style_bg_opa(img_list[idx], LV_OPA_20, 0);
        lv_obj_set_style_radius(img_list[idx], 10, 0);
    }

    /* ===== 3. ???? ===== */
    if(ta && kb)
    {
        lv_obj_clear_flag(ta, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_textarea_set_text(ta, "");
    }

    /* ===== 4. ???? ===== */
    if(is_decimal[idx] == 0)
        lv_btnmatrix_set_map(kb, kb_map_int);
    else
        lv_btnmatrix_set_map(kb, kb_map_float);

    /* ===== 5. ?? ===== */
    lv_obj_move_foreground(kb);
    lv_obj_move_foreground(ta);
}

/* ===================== ???? ===================== */
void kb_event_cb(lv_event_t * e)
{
    /* 1?? ???? */
    if(lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;

    /* 2?? ?????? */
    uint16_t btn_id = lv_btnmatrix_get_selected_btn(kb);
    const char *txt = lv_btnmatrix_get_btn_text(kb, btn_id);
    if(txt == NULL) return;

    /* 3?? ????? */
    char buf[32];
    strncpy(buf, lv_textarea_get_text(ta), sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    /* ===================== ?? ?? ===================== */
    if(strcmp(txt, "DEL") == 0)
    {
        size_t len = strlen(buf);
        if(len > 0)
            buf[len - 1] = '\0';
    }

    /* ===================== ?? ?? ===================== */
    else if(strcmp(txt, "OK") == 0)
    {
        /* ???? */
        sanitize_input(buf);

        if(strlen(buf) == 0)
            strcpy(buf, "0");

        /* ???? */
        float qty = is_decimal[current_index] ? atof(buf) : (float)atoi(buf);

        /* ???? */
        float raw_total = qty * prices[current_index];
        float total = raw_total;

        /* ===================== ?? ???? ===================== */
        if(current_index == 0)              // 10% OFF
        {
            total *= 0.9f;
        }
        else if(current_index == 3)         // ?20?5
        {
            if(raw_total >= 20.0f)
                total -= 5.0f;
        }

        /* ????? */
        cart_qty[current_index] = qty;

        /* ===================== ??? UI?? ===================== */
        char b1[32], b2[64];

        /* ?? */
        snprintf(b1, sizeof(b1), "Qty: %.2f", qty);
        lv_label_set_text(qty_label[current_index], b1);

        /* ?? + ???? */
        if(current_index == 0)
        {
            snprintf(b2, sizeof(b2), "Total: %.2f (10%% OFF)", total);
        }
        else if(current_index == 3 && raw_total >= 20.0f)
        {
            snprintf(b2, sizeof(b2), "Total: %.2f (-5)", total);
        }
        else
        {
            snprintf(b2, sizeof(b2), "Total: %.2f", total);
        }

        lv_label_set_text(total_label[current_index], b2);

        /* ???? */
        update_total_sum();

        /* ===================== ?? ?? ===================== */
        lv_textarea_set_text(ta, "");
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ta, LV_OBJ_FLAG_HIDDEN);

        current_index = -1;   // ?? ?????

        return;
    }

    /* ===================== ?? ???? ===================== */
    else
    {
        /* ????? */
        strncat(buf, txt, sizeof(buf) - strlen(buf) - 1);
    }

    /* ===================== ?? ???? ===================== */
    sanitize_input(buf);

    /* ===================== ?? ????? ===================== */
    lv_textarea_set_text(ta, buf);

    /* ???? */
    lv_textarea_set_cursor_pos(ta, LV_TEXTAREA_CURSOR_LAST);
}

/* ===================== Screen2 ===================== */
static void checkout_btn_cb(lv_event_t *e)
{
    lv_msgbox_close(lv_event_get_current_target(e));
}

// ??????
static void show_checkout_popup_old(lv_event_t *e)
{
    float total_all = 0;
    for (int i = 0; i < 5; i++) {
        total_all += cart_qty[i] * prices[i];
    }

    char buf[128];
    sprintf(buf, "Total: %.2f", total_all);
    static const char *btns[] = {"Confirm", "Cancel", NULL};

    lv_obj_t *mbox = lv_msgbox_create(NULL, "Checkout", buf, btns, true);
    lv_obj_center(mbox);
    lv_obj_add_event_cb(mbox, checkout_btn_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

// ???????
void create_scr2(void)
{
    scr2 = lv_obj_create(NULL);

    // ===================== ?????? =====================
    lv_obj_t *btn_cont = lv_obj_create(scr2);
    lv_obj_set_size(btn_cont, 100, 300);
    lv_obj_align(btn_cont, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_set_style_bg_opa(btn_cont, 0, 0); // ????

    // ????
    lv_obj_t *btn_back = lv_btn_create(btn_cont);
    lv_obj_set_size(btn_back, 90, 40);
    lv_obj_align(btn_back, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, "Back");
    lv_obj_center(lbl_back);
    lv_obj_add_event_cb(btn_back, switch_btn_event_cb, LV_EVENT_CLICKED, NULL);

    // ????
    lv_obj_t *btn_check = lv_btn_create(btn_cont);
    lv_obj_set_size(btn_check, 90, 40);
    lv_obj_align(btn_check, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_t *lbl_check = lv_label_create(btn_check);
    lv_label_set_text(lbl_check, "Pay");
    lv_obj_center(lbl_check);
    lv_obj_add_event_cb(btn_check, show_checkout_popup, LV_EVENT_CLICKED, NULL);

    // ===================== ???????(100%????LVGL) =====================
    cart_list = lv_obj_create(scr2);
    // ??:???? - 120(????????)
    // ??:???? - 20(????)
    lv_obj_set_size(cart_list, lv_disp_get_hor_res(NULL) - 120, lv_disp_get_ver_res(NULL) - 20);
    lv_obj_align(cart_list, LV_ALIGN_LEFT_MID, 10, 0);

    // ???????????????
    lv_obj_set_scroll_dir(cart_list, LV_DIR_VER);
    // ???
    lv_obj_set_style_pad_all(cart_list, 10, 0);
    // ???????(?????????)
    //lv_obj_set_style_scrollbar_mode(cart_list, LV_SCROLLBAR_MODE_ON, LV_PART_MAIN);
}


// ===================== 创建交易记录 =====================
void create_new_record(void)
{
    // ---------- 优化点：防止数组溢出 ----------
    if (record_count >= MAX_RECORDS) return;

    const char *item_names[5] = {"milk", "chips", "bread", "apple", "water"};
    char items_buf[256] = {0}; // 缓冲区存商品信息
    float total_all = 0.0f;

    size_t buf_remaining = sizeof(items_buf);
    char *buf_ptr = items_buf;

    // ---------- 优化点：安全拼接字符串 ----------
    for (int i = 0; i < 5; i++) {
        if (cart_qty[i] <= 0.001f) continue;

        total_all += cart_qty[i] * prices[i];

        int written = snprintf(buf_ptr, buf_remaining, "%s:%.2f  ", item_names[i], cart_qty[i]);
        if (written < 0 || (size_t)written >= buf_remaining) {
            buf_ptr[buf_remaining - 1] = '\0'; // 确保字符串结尾
            break;
        }
        buf_ptr += written;
        buf_remaining -= written;
    }

    // ---------- 优化点：空购物车不生成记录 ----------
    if (strlen(items_buf) == 0) return;

    // ---------- 优化点：倒序插入新记录 ----------
    for (int i = record_count; i > 0; i--) {
        record_list[i] = record_list[i - 1];
    }

    record_list[0].total = total_all;
    strncpy(record_list[0].items, items_buf, sizeof(record_list[0].items) - 1);
    record_list[0].items[sizeof(record_list[0].items) - 1] = '\0';
    record_count++;

    // ---------- 优化点：结账清空购物车 ----------
    memset(cart_qty, 0, sizeof(cart_qty));
    update_total_sum();

    // ---------- 优化点：实时刷新界面 ----------
    if (record_container) refresh_records();
}

// ===================== 刷新交易记录显示 =====================
static void refresh_records(void)
{
    if (!record_container) return;

    // ---------- 优化点：清空容器，防止重复显示 ----------
    lv_obj_clean(record_container);

    // ---------- 优化点：自动垂直布局 ----------
    lv_obj_set_flex_flow(record_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(record_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(record_container, 10, 0); // 行间距
    lv_obj_set_style_pad_all(record_container, 5, 0);

    for (int i = 0; i < record_count; i++) {
        // ---------- 优化点：每条记录用卡片式容器 ----------
        lv_obj_t *card = lv_obj_create(record_container);
        lv_obj_set_width(card, lv_disp_get_hor_res(NULL) - 40);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_border_color(card, lv_color_black(), 0);
        lv_obj_set_style_radius(card, 5, 0);
        lv_obj_set_style_pad_all(card, 8, 0);

        // 商品信息
        lv_obj_t *lbl_item = lv_label_create(card);
        lv_label_set_text_fmt(lbl_item, "Items: %s", record_list[i].items);
        lv_label_set_long_mode(lbl_item, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(lbl_item, lv_obj_get_width(card) - 16); // 卡片内部边距

        // 总价信息
        lv_obj_t *lbl_total = lv_label_create(card);
        lv_label_set_text_fmt(lbl_total, "Total: %.2f", record_list[i].total);

        // ---------- 优化点：分割线隐藏在卡片边框内 ----------
        // 不再手动绘制线条，卡片边框即分割
    }
}

// ===================== 创建滚动交易记录页面 =====================
void create_scr3_records(void)
{
    scr3 = lv_obj_create(NULL);

    // 返回按钮
    lv_obj_t *btn_back = lv_btn_create(scr3);
    lv_obj_set_size(btn_back, 90, 40);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, "Back");
    lv_obj_center(lbl_back);
    lv_obj_add_event_cb(btn_back, switch_btn_event_cb, LV_EVENT_CLICKED, NULL);

    // 标题
    lv_obj_t *title = lv_label_create(scr3);
    lv_label_set_text(title, "Transaction Records");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    // 滚动容器
    record_container = lv_obj_create(scr3);
    lv_obj_set_size(record_container,
                    lv_disp_get_hor_res(NULL) - 20,
                    lv_disp_get_ver_res(NULL) - 70);
    lv_obj_align(record_container, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_scroll_dir(record_container, LV_DIR_VER);
    lv_obj_set_style_pad_all(record_container, 10, 0);

    refresh_records();
}

// ===================== 结账弹窗回调 =====================
static void checkout_msgbox_cb(lv_event_t *e)
{
    lv_obj_t *mbox = lv_event_get_current_target(e);
    uint32_t btn_idx = lv_msgbox_get_active_btn(mbox);

    if (btn_idx == 0) {  // Confirm
        create_new_record(); // ---------- 优化点：确认生成记录 ----------
    }

    lv_msgbox_close(mbox);
}

// ===================== 结账弹窗 =====================
#undef show_checkout_popup
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

/* ===================== Screen3 ===================== */
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

/* ===================== Screen1 ===================== */
#define ITEM_COUNT 5

typedef struct {
    const char *name;
    const char *img_path;
    float price;
    int is_decimal;
    const char *promo;
} product_t;

static const product_t products[ITEM_COUNT] = {
    {"milk",  "0:/test_image0.bin", 3, 0, "10% OFF"},
    {"chips", "0:/test_image1.bin", 7, 1, NULL},
    {"bread", "0:/test_image2.bin", 6, 1, NULL},
    {"apple", "0:/test_image3.bin", 5, 1, "Spend 20 Save 5"},
    {"water", "0:/test_image4.bin", 4, 0, NULL},
};

void create_scr1(void)
{
    scr1 = lv_obj_create(NULL);

    /* ===================== ???? ===================== */
    lv_obj_t *btn_cart = lv_btn_create(scr1);
    lv_obj_set_size(btn_cart, 90, 40);
    lv_obj_align(btn_cart, LV_ALIGN_TOP_LEFT, 10, 20);
    lv_obj_add_event_cb(btn_cart, switch_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl1 = lv_label_create(btn_cart);
    lv_label_set_text(lbl1, "Cart");
    lv_obj_center(lbl1);

    lv_obj_t *btn_records = lv_btn_create(scr1);
    lv_obj_set_size(btn_records, 110, 40);
    lv_obj_align(btn_records, LV_ALIGN_TOP_RIGHT, -10, 20);
    lv_obj_add_event_cb(btn_records, switch_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl2 = lv_label_create(btn_records);
    lv_label_set_text(lbl2, "Records");
    lv_obj_center(lbl2);

    /* ===================== ???? ===================== */
    static lv_img_dsc_t img_dsc[ITEM_COUNT];

    int w = lv_disp_get_hor_res(NULL) / ITEM_COUNT;
    int y_offset = 30;

    for(int i = 0; i < ITEM_COUNT; i++)
    {
        int x = i * w;

        /* ---- ???? ---- */
        uint8_t *buf = (uint8_t*)sdram_malloc(200*310*3+4);
        read_file_to_array(products[i].img_path, buf, 200*310*3+4);

        img_dsc[i].header.always_zero = 0;
        img_dsc[i].header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
        img_dsc[i].header.w = 200;
        img_dsc[i].header.h = 310;
        img_dsc[i].data = buf + 4;

        img_list[i] = lv_img_create(scr1);
        lv_img_set_src(img_list[i], &img_dsc[i]);
        lv_obj_set_pos(img_list[i], x + 5, y_offset + 40);

        lv_obj_add_flag(img_list[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(img_list[i], img_click_event_cb, LV_EVENT_CLICKED, (void*)i);

        /* ---- ???? ---- */
        int base_x = x + 50;
        int base_y = y_offset + 360;

        create_box_label(scr1, products[i].name,
            lv_palette_main(LV_PALETTE_BLUE), base_x, base_y);

        char price_buf[32];
        snprintf(price_buf, sizeof(price_buf),
            products[i].is_decimal ? "%.1f/kg" : "%.1f/item",
            products[i].price);

        create_box_label(scr1, price_buf,
            lv_palette_main(LV_PALETTE_GREEN), base_x, base_y + 30);

        qty_label[i] = create_box_label(scr1, "Qty:0",
            lv_palette_main(LV_PALETTE_ORANGE), base_x, base_y + 60);

        total_label[i] = create_box_label(scr1, "Total:0",
            lv_palette_main(LV_PALETTE_RED), base_x, base_y + 90);

        /* ---- ???? ---- */
        if(products[i].promo)
        {
            create_box_label(scr1, products[i].promo,
                lv_palette_main(LV_PALETTE_PURPLE),
                base_x, base_y + 120);
        }
    }

    /* ===================== ??? ===================== */
    ta = lv_textarea_create(scr1);
    lv_obj_set_size(ta, 150, 40);
    lv_obj_align(ta, LV_ALIGN_BOTTOM_MID, 0, -200);
    lv_obj_clear_flag(ta, LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_one_line(ta, true);
    lv_obj_add_flag(ta, LV_OBJ_FLAG_HIDDEN);

    /* ===================== ?? ===================== */
    kb = lv_btnmatrix_create(scr1);
    lv_obj_set_size(kb, 300, 200);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* ===================== ???? ===================== */
    lv_obj_add_flag(scr1, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(scr1, blank_click_event_cb, LV_EVENT_PRESSED, NULL);

    /* ===================== ??? ===================== */
    lv_obj_t *cont = lv_obj_create(scr1);
    lv_obj_set_size(cont, 220, 50);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_set_style_bg_color(cont, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_radius(cont, 12, 0);
    lv_obj_set_style_border_width(cont, 0, 0);

    total_sum_label = lv_label_create(cont);
    lv_label_set_text(total_sum_label, "SUM: 0.00");
    lv_obj_set_style_text_color(total_sum_label, lv_color_white(), 0);
    lv_obj_center(total_sum_label);
}
