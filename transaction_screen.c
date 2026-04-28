#include "transaction_screen.h"
#include <stdio.h>
#include <string.h>

lv_obj_t *scr3;

#define MAX_RECORDS 50
#define MAX_ITEMS_PER_RECORD 5

typedef struct {
    char name[32];      // 商品名称
    float quantity;     // 数量
    float price;        // 单价
    float subtotal;     // 小计（折扣后）
    float raw_subtotal; // 原始小计（折扣前）
} ItemDetail;

typedef struct {
    ItemDetail items[MAX_ITEMS_PER_RECORD];  // 商品明细数组
    int item_count;                           // 商品数量
    float total;                              // 总价
} Record;

static Record record_list[MAX_RECORDS];
static int record_count = 0;
static lv_obj_t *record_container = NULL;

extern float cart_qty[5];
extern void update_total_sum(void);
extern lv_obj_t *scr1;

static const float prices[5] = {3, 7, 6, 5, 4};

void refresh_records(void)
{
    if (!record_container) return;

    // 清空容器
    lv_obj_clean(record_container);
    
    // 启用垂直滚动，以便查看多条记录
    lv_obj_set_scroll_dir(record_container, LV_DIR_VER);
    lv_obj_set_style_pad_all(record_container, 10, 0);

    // 如果没有记录，显示提示信息
    if (record_count == 0) {
        lv_obj_t *lbl = lv_label_create(record_container);
        lv_label_set_text(lbl, "No transaction records");
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
        return;
    }

    int y_offset = 10;  // 从顶部开始
    
    // 定义颜色数组，循环使用
    lv_color_t colors[] = {
        lv_palette_main(LV_PALETTE_BLUE),
        lv_palette_main(LV_PALETTE_GREEN),
        lv_palette_main(LV_PALETTE_ORANGE),
        lv_palette_main(LV_PALETTE_RED),
        lv_palette_main(LV_PALETTE_PURPLE),
        lv_palette_main(LV_PALETTE_CYAN),
        lv_palette_main(LV_PALETTE_PINK),
        lv_palette_main(LV_PALETTE_INDIGO)
    };
    int color_count = sizeof(colors) / sizeof(colors[0]);

    for (int i = 0; i < record_count; i++) {
        // 创建彩色背景的卡片容器
        lv_obj_t *card = lv_obj_create(record_container);
        lv_obj_set_size(card, lv_disp_get_hor_res(NULL) - 40, LV_SIZE_CONTENT);  // 宽度固定，高度自适应
        lv_obj_align(card, LV_ALIGN_TOP_MID, 0, y_offset);
        lv_obj_set_style_border_width(card, 0, 0);  // 去掉边框
        lv_obj_set_style_radius(card, 10, 0);
        lv_obj_set_style_pad_all(card, 12, 0);  // 增加内边距
        
        // 设置彩色背景
        lv_obj_set_style_bg_color(card, colors[i % color_count], 0);
        lv_obj_set_style_bg_opa(card, LV_OPA_30, 0);  // 设置透明度，让颜色更柔和
        
        // 禁用每个card的滚动
        lv_obj_set_scroll_dir(card, LV_DIR_NONE);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        
        int card_y_offset = 0;  // 卡片内部的垂直偏移
        
        // 显示记录序号标签
        char count_buf[32];
        sprintf(count_buf, "Record %d:", i + 1);
        lv_obj_t *lbl_count = lv_label_create(card);
        lv_label_set_text(lbl_count, count_buf);
        lv_obj_align(lbl_count, LV_ALIGN_TOP_LEFT, 8, card_y_offset);
        card_y_offset += 25;
        
        // 显示商品明细
        for (int j = 0; j < record_list[i].item_count; j++) {
            char item_buf[128];
            int qty_int = (int)record_list[i].items[j].quantity;
            int qty_frac = (int)((record_list[i].items[j].quantity - qty_int) * 100 + 0.5f);
            if (qty_frac < 0) qty_frac = -qty_frac;
            if (qty_frac >= 100) qty_frac = 0;
            
            int sub_int = (int)record_list[i].items[j].subtotal;
            int sub_frac = (int)((record_list[i].items[j].subtotal - sub_int) * 100 + 0.5f);
            if (sub_frac < 0) sub_frac = -sub_frac;
            if (sub_frac >= 100) sub_frac = 0;
            
            // 检查是否有折扣
            float diff = record_list[i].items[j].raw_subtotal - record_list[i].items[j].subtotal;
            if (diff > 0.01f) {
                // 有折扣，显示折扣信息
                int raw_int = (int)record_list[i].items[j].raw_subtotal;
                int raw_frac = (int)((record_list[i].items[j].raw_subtotal - raw_int) * 100 + 0.5f);
                if (raw_frac < 0) raw_frac = -raw_frac;
                if (raw_frac >= 100) raw_frac = 0;
                
                int price_int = (int)record_list[i].items[j].price;
                sprintf(item_buf, "%s: %d.%02d x %d = %d.%02d (Original Price: %d.%02d)", 
                       record_list[i].items[j].name,
                       qty_int, qty_frac,
                       price_int,
                       sub_int, sub_frac,
                       raw_int, raw_frac);
            } else {
                // 没有折扣，正常显示
                int price_int = (int)record_list[i].items[j].price;
                sprintf(item_buf, "%s: %d.%02d x %d = %d.%02d", 
                       record_list[i].items[j].name,
                       qty_int, qty_frac,
                       price_int,
                       sub_int, sub_frac);
            }
            
            lv_obj_t *lbl_item = lv_label_create(card);
            lv_label_set_text(lbl_item, item_buf);
            lv_obj_align(lbl_item, LV_ALIGN_TOP_LEFT, 15, card_y_offset);
            card_y_offset += 22;
        }
        
        // 显示总价
        lv_obj_t *lbl_total = lv_label_create(card);
        char total_buf[64];
        int total_int = (int)record_list[i].total;
        int total_frac = (int)((record_list[i].total - total_int) * 100 + 0.5f);
        if (total_frac < 0) total_frac = -total_frac;
        if (total_frac >= 100) total_frac = 0;
        sprintf(total_buf, "Total: %d.%02d", total_int, total_frac);
        lv_label_set_text(lbl_total, total_buf);
        lv_obj_align(lbl_total, LV_ALIGN_BOTTOM_MID, 0, -8);
        
        y_offset += 120;  // 每个card间隔120像素（根据内容动态调整）
    }
}

void create_new_record(void)
{
    if (record_count >= MAX_RECORDS) return;

    const char *item_names[5] = {"milk", "chips", "bread", "apple", "water"};
    float total_all = 0.0f;
    int item_idx = 0;

    // 先将所有现有记录后移一位（为新记录腾出空间）
    for (int i = record_count; i > 0; i--) {
        record_list[i] = record_list[i - 1];
    }

    // 清空新记录位置
    memset(&record_list[0], 0, sizeof(Record));

    for (int i = 0; i < 5; i++) {
        if (cart_qty[i] <= 0.001f) continue;

        float raw_total = cart_qty[i] * prices[i];
        float final_total = raw_total;

        // 应用折扣规则（与product_screen.c一致）
        if (i == 0)  // milk: 10% OFF
        {
            final_total = raw_total * 0.9f;
        }
        else if (i == 3)  // apple: 满20减5
        {
            if (raw_total >= 20.0f)
                final_total = raw_total - 5.0f;
        }

        total_all += final_total;

        // 保存商品明细
        if (item_idx < MAX_ITEMS_PER_RECORD) {
            strncpy(record_list[0].items[item_idx].name, item_names[i], 
                   sizeof(record_list[0].items[item_idx].name) - 1);
            record_list[0].items[item_idx].quantity = cart_qty[i];
            record_list[0].items[item_idx].price = prices[i];
            record_list[0].items[item_idx].subtotal = final_total;  // 保存折扣后的价格
            record_list[0].items[item_idx].raw_subtotal = raw_total;  // 保存原始价格
            item_idx++;
        }
    }

    if (item_idx == 0) {
        // 没有商品，恢复原来的记录顺序
        for (int i = 0; i < record_count; i++) {
            record_list[i] = record_list[i + 1];
        }
        return;
    }

    record_list[0].item_count = item_idx;
    record_list[0].total = total_all;

    record_count++;

    memset(cart_qty, 0, sizeof(cart_qty));
    update_total_sum();

    // 无论record_container是否存在，都尝试刷新
    // 如果不存在，等scr3显示时会调用create_scr3_records()来初始化
    refresh_records();
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
        // 返回前先刷新一次，确保显示最新数据
        if(record_container) {
            refresh_records();
        }
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

    // 创建标题按钮（和Back按钮一样的效果）
    lv_obj_t *title_btn = lv_btn_create(scr3);
    lv_obj_set_size(title_btn, 200, 40);
    lv_obj_align(title_btn, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_t *title = lv_label_create(title_btn);
    lv_label_set_text(title, "Transaction Records");
    lv_obj_center(title);

    record_container = lv_obj_create(scr3);
    lv_obj_set_size(record_container,
                    lv_disp_get_hor_res(NULL) - 20,
                    lv_disp_get_ver_res(NULL) - 70);
    lv_obj_align(record_container, LV_ALIGN_BOTTOM_MID, 0, -5);
    // 启用垂直滚动功能
    lv_obj_set_scroll_dir(record_container, LV_DIR_VER);
    lv_obj_set_style_pad_all(record_container, 10, 0);

    // 确保每次都刷新显示
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
