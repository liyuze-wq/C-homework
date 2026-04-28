#include "product_screen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "read_file_to_array.h"
#include "sdram_malloc.h"

lv_obj_t *scr1;

static lv_obj_t *kb;
static lv_obj_t *ta;
static lv_obj_t *qty_label[5];
static lv_obj_t *total_label[5];
static lv_obj_t *total_sum_label;
static lv_obj_t *img_list[5];

static int current_index = -1;
static const float prices[5] = {3, 7, 6, 5, 4};
static const int is_decimal[5] = {0, 1, 1, 1, 0};
float cart_qty[5] = {0.0f};

extern void refresh_cart_list(void);
extern lv_obj_t *scr2;
extern lv_obj_t *scr3;

// 辅助函数：将浮点数格式化为字符串（避免使用%.2f）
static void format_float(char *buf, size_t buf_size, float value)
{
    int int_part = (int)value;
    int frac_part = (int)((value - int_part) * 100 + 0.5f);
    if (frac_part < 0) frac_part = -frac_part;
    if (frac_part >= 100) frac_part = 0;
    snprintf(buf, buf_size, "%d.%02d", int_part, frac_part);
}

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

static bool is_point_in_obj(lv_obj_t *obj, lv_point_t *p)
{
    lv_area_t area;
    lv_obj_get_coords(obj, &area);
    return (p->x >= area.x1 && p->x <= area.x2 &&
            p->y >= area.y1 && p->y <= area.y2);
}

static void sanitize_input(char *buf)
{
    int len = strlen(buf);
    int dot_count = 0;
    char new_buf[32] = {0};
    int j = 0;

    for(int i = 0; i < len; i++)
    {
        char c = buf[i];
        if((c >= '0' && c <= '9') || c == '.')
        {
            if(is_decimal[current_index] == 0 && c == '.')
                continue;
            if(c == '.')
            {
                dot_count++;
                if(dot_count > 1) continue;
            }
            if(j < sizeof(new_buf) - 1)
            {
                new_buf[j] = c;
                j++;
            }
        }
    }

    new_buf[j] = '\0';

    if(new_buf[0] == '.')
    {
        memmove(new_buf+1, new_buf, strlen(new_buf)+1);
        new_buf[0] = '0';
    }

    char *dot = strchr(new_buf, '.');
    if(dot)
    {
        if(strlen(dot) > 3)
           if(is_decimal[current_index])
        {
            float v = atof(new_buf);
            // 使用整数格式化代替浮点数格式化
            int v_int = (int)v;
            int v_frac = (int)((v - v_int) * 100 + 0.5f);
            if (v_frac < 0) v_frac = -v_frac;
            if (v_frac >= 100) v_frac = 0;
            sprintf(new_buf, "%d.%02d", v_int, v_frac);
        }
    }

    float val = atof(new_buf);
    if(val > 99.99f)
    {
        strcpy(new_buf, "99.99");
    }

    strcpy(buf,new_buf);
}

void update_total_sum(void)
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
    // 使用整数格式化代替浮点数格式化
    int sum_int = (int)sum;
    int sum_frac = (int)((sum - sum_int) * 100 + 0.5f);
    if (sum_frac < 0) sum_frac = -sum_frac;
    if (sum_frac >= 100) sum_frac = 0;
    sprintf(buf, "SUM: %d.%02d", sum_int, sum_frac);
    lv_label_set_text(total_sum_label, buf);
}

static void clear_img_selection_anim(void)
{
    for(int i = 0; i < 5; i++)
    {
        if(img_list[i] == NULL) continue;
        lv_anim_t a1;
        lv_anim_init(&a1);
        lv_anim_set_var(&a1, img_list[i]);
        lv_anim_set_values(&a1, LV_OPA_20, LV_OPA_TRANSP);
        lv_anim_set_time(&a1, 150);
        lv_anim_set_exec_cb(&a1, (lv_anim_exec_xcb_t)lv_obj_set_style_bg_opa);
        lv_anim_start(&a1);
        lv_obj_set_style_border_width(img_list[i], 0, 0);
        lv_obj_set_style_border_color(img_list[i], lv_color_black(), 0);
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

    lv_textarea_set_text(ta, "");

    lv_anim_t a_kb;
    lv_anim_init(&a_kb);
    lv_anim_set_var(&a_kb, kb);
    lv_anim_set_values(&a_kb, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a_kb, 200);
    lv_anim_set_exec_cb(&a_kb, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_start(&a_kb);

    lv_anim_t a_ta;
    lv_anim_init(&a_ta);
    lv_anim_set_var(&a_ta, ta);
    lv_anim_set_values(&a_ta, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a_ta, 200);
    lv_anim_set_exec_cb(&a_ta, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_start(&a_ta);

    clear_img_selection_anim();

    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ta, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_opa(kb, LV_OPA_COVER, 0);
    lv_obj_set_style_opa(ta, LV_OPA_COVER, 0);

    current_index = -1;
}

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

static void switch_btn_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    lv_obj_t *btn = lv_event_get_target(e);
    if(btn == NULL) return;
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    if(label == NULL) return;
    const char *txt = lv_label_get_text(label);
    if(txt == NULL) return;

    if(strcmp(txt, "Cart") == 0)
    {
        lv_scr_load(scr2);
        refresh_cart_list();
    }
    else if(strcmp(txt, "Records") == 0)
    {
        lv_scr_load(scr3);
    }
}

static void img_click_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    int idx = (int)lv_event_get_user_data(e);
    if(idx < 0 || idx >= 5) return;

    current_index = idx;
    clear_img_selection_anim();

    if(img_list[idx])
    {
        lv_obj_set_style_border_width(img_list[idx], 3, 0);
        lv_obj_set_style_border_color(img_list[idx],
            lv_palette_main(LV_PALETTE_RED), 0);
        lv_obj_set_style_bg_opa(img_list[idx], LV_OPA_20, 0);
        lv_obj_set_style_radius(img_list[idx], 10, 0);
    }

    if(ta && kb)
    {
        lv_obj_clear_flag(ta, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_textarea_set_text(ta, "");
    }

    if(is_decimal[idx] == 0)
        lv_btnmatrix_set_map(kb, kb_map_int);
    else
        lv_btnmatrix_set_map(kb, kb_map_float);

    lv_obj_move_foreground(kb);
    lv_obj_move_foreground(ta);
}

void kb_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;

    uint16_t btn_id = lv_btnmatrix_get_selected_btn(kb);
    const char *txt = lv_btnmatrix_get_btn_text(kb, btn_id);
    if(txt == NULL) return;

    char buf[32];
    strncpy(buf, lv_textarea_get_text(ta), sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    if(strcmp(txt, "DEL") == 0)
    {
        size_t len = strlen(buf);
        if(len > 0)
            buf[len - 1] = '\0';
    }
    else if(strcmp(txt, "OK") == 0)
    {
        sanitize_input(buf);
        if(strlen(buf) == 0)
            strcpy(buf, "0");

        float qty = is_decimal[current_index] ? atof(buf) : (float)atoi(buf);
        float raw_total = qty * prices[current_index];
        float total = raw_total;

        if(current_index == 0)
        {
            total *= 0.9f;
        }
        else if(current_index == 3)
        {
            if(raw_total >= 20.0f)
                total -= 5.0f;
        }

        cart_qty[current_index] = qty;

        char b1[32], b2[64];
        format_float(b1, sizeof(b1), qty);
        // 添加"Qty: "前缀
        char b1_full[64];
        snprintf(b1_full, sizeof(b1_full), "Qty: %s", b1);
        lv_label_set_text(qty_label[current_index], b1_full);

        char total_str[32];
        format_float(total_str, sizeof(total_str), total);
        
        if(current_index == 0)
        {
            snprintf(b2, sizeof(b2), "Total: %s (10%% OFF)", total_str);
        }
        else if(current_index == 3 && raw_total >= 20.0f)
        {
            snprintf(b2, sizeof(b2), "Total: %s (-5)", total_str);
        }
        else
        {
            snprintf(b2, sizeof(b2), "Total: %s", total_str);
        }

        lv_label_set_text(total_label[current_index], b2);
        update_total_sum();

        lv_textarea_set_text(ta, "");
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ta, LV_OBJ_FLAG_HIDDEN);
        current_index = -1;
        return;
    }
    else
    {
        strncat(buf, txt, sizeof(buf) - strlen(buf) - 1);
    }

    sanitize_input(buf);
    lv_textarea_set_text(ta, buf);
    lv_textarea_set_cursor_pos(ta, LV_TEXTAREA_CURSOR_LAST);
}

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

    static lv_img_dsc_t img_dsc[ITEM_COUNT];
    int w = lv_disp_get_hor_res(NULL) / ITEM_COUNT;
    int y_offset = 30;

    for(int i = 0; i < ITEM_COUNT; i++)
    {
        int x = i * w;

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

        if(products[i].promo)
        {
            create_box_label(scr1, products[i].promo,
                lv_palette_main(LV_PALETTE_PURPLE),
                base_x, base_y + 120);
        }
    }

    ta = lv_textarea_create(scr1);
    lv_obj_set_size(ta, 150, 40);
    lv_obj_align(ta, LV_ALIGN_BOTTOM_MID, 0, -200);
    lv_obj_clear_flag(ta, LV_OBJ_FLAG_SCROLLABLE);
    lv_textarea_set_one_line(ta, true);
    lv_obj_add_flag(ta, LV_OBJ_FLAG_HIDDEN);

    kb = lv_btnmatrix_create(scr1);
    lv_obj_set_size(kb, 300, 200);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_add_flag(scr1, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(scr1, blank_click_event_cb, LV_EVENT_PRESSED, NULL);

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
