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
    char time_str[32];                        // 交易时间
} Record;

static Record record_list[MAX_RECORDS];
static int record_count = 0;
static lv_obj_t *record_container = NULL;
static lv_obj_t *stats_label = NULL;  // 统计信息标签
static float total_sales = 0.0f;  // 总销售额统计

extern float cart_qty[5];
extern void update_total_sum(void);
extern lv_obj_t *scr1;

static const float prices[5] = {3, 7, 6, 5, 4};

// 更新顶部统计信息
static void update_stats_label(void)
{
    if (stats_label == NULL) return;
    
    char stats_buf[64];
    int sales_int = (int)total_sales;
    int sales_frac = (int)((total_sales - sales_int) * 100 + 0.5f);
    if (sales_frac < 0) sales_frac = -sales_frac;
    if (sales_frac >= 100) sales_frac = 0;
    sprintf(stats_buf, "Sales: $%d.%02d | %d txns", sales_int, sales_frac, record_count);
    lv_label_set_text(stats_label, stats_buf);
}

void refresh_records(void)
{
    if (!record_container) return;

    // 更新统计信息
    update_stats_label();

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
    
    // 定义颜色数组，循环使用（去掉蓝色，使用与冷色调背景有区别的颜色）
    lv_color_t colors[] = {
        lv_palette_main(LV_PALETTE_GREEN),      // 绿色
        lv_palette_main(LV_PALETTE_ORANGE),     // 橙色
        lv_palette_main(LV_PALETTE_RED),        // 红色
        lv_palette_main(LV_PALETTE_PURPLE),     // 紫色
        lv_palette_main(LV_PALETTE_PINK),       // 粉色
        lv_palette_main(LV_PALETTE_INDIGO),     // 靛蓝
        lv_palette_main(LV_PALETTE_BROWN),      // 棕色
        lv_palette_main(LV_PALETTE_TEAL)        // 青色
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
        char count_buf[64];
        sprintf(count_buf, "%s (Record %d):", record_list[i].time_str, i + 1);
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
        
        // 动态计算卡片高度并更新y_offset
        // 卡片高度 = card_y_offset（内容总高度）+ 上下内边距(12*2) + 底部间距
        int card_height = card_y_offset + 24 + 30;  // 24是上下内边距，30是底部额外空间
        y_offset += card_height + 10;  // 每条记录间隔10像素
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

    // 获取当前交易时间（简化版，使用递增计数器模拟）
    static int transaction_counter = 0;
    transaction_counter++;
    sprintf(record_list[0].time_str, "Transaction #%d", transaction_counter);

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
    total_sales += total_all;  // 累加销售额

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
    // 设置暖色调背景（浅粉橙色）
    lv_obj_set_style_bg_color(scr3, lv_color_hex(0xFFE4E1), 0);
    lv_obj_set_style_bg_opa(scr3, LV_OPA_COVER, 0);

    // 创建顶部标题标签方框 - 使用契合暖色调的橙红色
    lv_obj_t *title_box = lv_obj_create(scr3);
    lv_obj_set_size(title_box, 220, 40);  // 适中的大小
    lv_obj_align(title_box, LV_ALIGN_TOP_MID, 0, 15);
    lv_obj_set_style_bg_color(title_box, lv_color_hex(0xFF7043), 0);  // 橙红色渐变起始色
    lv_obj_set_style_bg_grad_color(title_box, lv_color_hex(0xF4511E), 0);  // 橙红色渐变结束色
    lv_obj_set_style_bg_grad_dir(title_box, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_radius(title_box, 10, 0);
    lv_obj_set_style_border_width(title_box, 0, 0);
    lv_obj_set_style_shadow_width(title_box, 6, 0);
    lv_obj_set_style_shadow_color(title_box, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(title_box, LV_OPA_30, 0);
    lv_obj_set_style_shadow_ofs_y(title_box, 2, 0);
    // 禁用标题方框的滚动
    lv_obj_set_scroll_dir(title_box, LV_DIR_NONE);
    lv_obj_clear_flag(title_box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(title_box);
    lv_label_set_text(title, "Transaction Records");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);  // 白色字体
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_set_scroll_dir(title, LV_DIR_NONE);
    lv_obj_clear_flag(title, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(title);

    // Back按钮 - 使用契合暖色调的棕色（直接放在scr3上）
    lv_obj_t *btn_back = lv_btn_create(scr3);
    lv_obj_set_size(btn_back, 80, 40);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 20, 15);  // 上移到与标题同一高度
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x8D6E63), 0);  // 棕色渐变起始色
    lv_obj_set_style_bg_grad_color(btn_back, lv_color_hex(0x6D4C41), 0);  // 棕色渐变结束色
    lv_obj_set_style_bg_grad_dir(btn_back, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_radius(btn_back, 10, 0);
    lv_obj_set_style_shadow_width(btn_back, 5, 0);
    lv_obj_set_style_shadow_opa(btn_back, LV_OPA_30, 0);
    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, "Back");
    lv_obj_center(lbl_back);
    lv_obj_add_event_cb(btn_back, switch_btn_event_cb, LV_EVENT_CLICKED, NULL);

    // 统计信息 - 使用契合暖色调的深橙色标签方框（直接放在scr3上）
    lv_obj_t *stats_box = lv_obj_create(scr3);
    lv_obj_set_size(stats_box, 180, 40);  // 固定宽度，与Back按钮和标题协调
    lv_obj_align(stats_box, LV_ALIGN_TOP_RIGHT, -20, 15);  // 上移到与标题同一高度
    lv_obj_set_style_bg_color(stats_box, lv_color_hex(0xFF9800), 0);  // 深橙色渐变起始色
    lv_obj_set_style_bg_grad_color(stats_box, lv_color_hex(0xF57C00), 0);  // 深橙色渐变结束色
    lv_obj_set_style_bg_grad_dir(stats_box, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_radius(stats_box, 8, 0);
    lv_obj_set_style_border_width(stats_box, 0, 0);
    lv_obj_set_style_shadow_width(stats_box, 3, 0);
    lv_obj_set_style_shadow_opa(stats_box, LV_OPA_30, 0);
    // 禁用滚动
    lv_obj_set_scroll_dir(stats_box, LV_DIR_NONE);
    lv_obj_clear_flag(stats_box, LV_OBJ_FLAG_SCROLLABLE);

    stats_label = lv_label_create(stats_box);
    char stats_buf[64];
    int sales_int = (int)total_sales;
    int sales_frac = (int)((total_sales - sales_int) * 100 + 0.5f);
    if (sales_frac < 0) sales_frac = -sales_frac;
    if (sales_frac >= 100) sales_frac = 0;
    sprintf(stats_buf, "Sales: $%d.%02d | %d txns", sales_int, sales_frac, record_count);
    lv_label_set_text(stats_label, stats_buf);
    lv_obj_set_style_text_color(stats_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(stats_label, &lv_font_montserrat_14, 0);
    lv_obj_set_scroll_dir(stats_label, LV_DIR_NONE);
    lv_obj_clear_flag(stats_label, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(stats_label);

    record_container = lv_obj_create(scr3);
    lv_obj_set_size(record_container,
                    lv_disp_get_hor_res(NULL) - 20,
                    lv_disp_get_ver_res(NULL) - 90);
    lv_obj_align(record_container, LV_ALIGN_BOTTOM_MID, 0, -10);
    // 启用垂直滚动功能
    lv_obj_set_scroll_dir(record_container, LV_DIR_VER);
    lv_obj_set_style_pad_all(record_container, 10, 0);
    // 设置冷色调背景（更冷的浅青色），与暖色调页面形成鲜明对比
    lv_obj_set_style_bg_color(record_container, lv_color_hex(0xB2EBF2), 0);  // 更冷的浅青色
    lv_obj_set_style_bg_opa(record_container, LV_OPA_COVER, 0);

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
