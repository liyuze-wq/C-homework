#include "cart_screen.h"
#include "transaction_screen.h"
#include "read_file_to_array.h"
#include "sdram_malloc.h"
#include <stdio.h>
#include <string.h>

lv_obj_t *scr2;

static lv_obj_t *cart_list = NULL;
static const float prices[5] = {3, 7, 6, 5, 4};

extern void create_new_record(void);
extern void update_total_sum(void);
extern lv_obj_t *scr1;

// 前向声明
static void clear_confirm_cb(lv_event_t *e);

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

// 检查购物车是否为空
static bool is_cart_empty(void)
{
    for (int i = 0; i < 5; i++) {
        if (cart_qty[i] > 0.0f) {
            return false;
        }
    }
    return true;
}

void refresh_cart_list(void)
{
    clear_cart_list();
    
    // 检查购物车是否为空
    bool is_empty = true;
    for (int i = 0; i < 5; i++) {
        if (cart_qty[i] > 0.0f) {
            is_empty = false;
            break;
        }
    }
    
    // 如果购物车为空，显示提示信息
    if (is_empty) {
        lv_obj_t *lbl = lv_label_create(cart_list);
        lv_label_set_text(lbl, "Cart is empty");
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x757575), 0);  // 灰色文字
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
        return;
    }
    
    const char *names[5] = {"milk","chips","bread","apple","water"};
    const char *paths[5] = 
    {
        "0:/test_image0.bin",
        "0:/test_image1.bin",
        "0:/test_image2.bin",
        "0:/test_image3.bin",
        "0:/test_image4.bin"
    };
    // 单位数组：0表示按件，1表示按重量(kg)
    const char *units[5] = {"item", "kg", "kg", "kg", "item"};
    
    int x = 10;
    int y = 10;
    int count = 0;
    static lv_img_dsc_t img_dsc[5];
    
    // 计算右侧图片区域的起始位置
    int screen_width = lv_disp_get_hor_res(NULL);
    int img_area_x = x + 660;  // Image标签在Total右边200像素（与Qty到Total的间距一致）
    // 图片高度约97像素（310*80/256≈97），标签高度30像素，让底部对齐
    int img_y = y + 30 - 97;  // 标签Y + 标签高度 - 图片高度

    // 表头 - 使用更深的颜色，与冷色调背景形成对比
    create_box_label(cart_list, "Product", lv_color_hex(0x1565C0), x, y);  // 深蓝色
    create_box_label(cart_list, "Qty", lv_color_hex(0xE65100), x+250, y);  // 深橙色
    create_box_label(cart_list, "Total", lv_color_hex(0xC62828), x+450, y);  // 深红色
    create_box_label(cart_list, "Image", lv_color_hex(0x6A1B9A), img_area_x, y);  // 深紫色，与Total间距200像素
    y += 75;

    // 遍历5个商品，只显示数量>0的
    for (int i = 0; i < 5; i++)
    {
        if (cart_qty[i] <= 0.0f)
        {   
            continue;
        }
        
        count++;
        
        char qbuf[32], tbuf[32];
        
        // 使用整数格式化代替浮点数格式化
        int qty_int = (int)cart_qty[i];
        int qty_frac = (int)((cart_qty[i] - qty_int) * 100 + 0.5f);
        if (qty_frac < 0) qty_frac = -qty_frac;
        if (qty_frac >= 100) qty_frac = 0;
        sprintf(qbuf, "%d.%02d %s", qty_int, qty_frac, units[i]);  // 添加单位
        
        float total_val = cart_qty[i] * prices[i];
        int total_int = (int)total_val;
        int total_frac = (int)((total_val - total_int) * 100 + 0.5f);
        if (total_frac < 0) total_frac = -total_frac;
        if (total_frac >= 100) total_frac = 0;
        sprintf(tbuf, "%d.%02d", total_int, total_frac);
        
        // 加载并显示图片（放在右侧）
        uint8_t *buf = (uint8_t*)sdram_malloc(200*310*3+4);
        read_file_to_array(paths[i], buf, 200*310*3+4);

        img_dsc[i].header.always_zero = 0;
        img_dsc[i].header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
        img_dsc[i].header.w = 200;
        img_dsc[i].header.h = 310;
        img_dsc[i].data = buf + 4;

        lv_obj_t *img = lv_img_create(cart_list);
        lv_img_set_src(img, &img_dsc[i]);
        lv_img_set_zoom(img, 80);  // 放大图片（从64改为80）
        // 图片位置设置为600
        lv_obj_set_pos(img, 620, img_y);

        // 显示商品信息（使用与表头一致的颜色）
        create_box_label(cart_list, names[i], lv_color_hex(0x1565C0), x, y);  // 深蓝色
        create_box_label(cart_list, qbuf, lv_color_hex(0xE65100), x+250, y);  // 深橙色
        create_box_label(cart_list, tbuf, lv_color_hex(0xC62828), x+450, y);  // 深红色

        y += 125;  // 垂直间距
        img_y += 125;  // 图片也向下移动
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

static void clear_cart_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    
    // 检查购物车是否为空
    if (is_cart_empty()) {
        lv_obj_t *mbox = lv_msgbox_create(NULL, "Notice", 
                                          "No items to clear in cart.", 
                                          NULL, true);
        lv_obj_center(mbox);
        lv_obj_set_style_bg_color(mbox, lv_color_hex(0xFFF3E0), 0);
        lv_obj_set_style_text_color(lv_msgbox_get_text(mbox), lv_color_hex(0xE65100), 0);
        return;
    }
    
    // 创建确认对话框
    static const char *btns[] = {"Cancel", "Clear", ""};
    lv_obj_t *mbox = lv_msgbox_create(NULL, "Confirm", "Clear all items from cart?", btns, true);
    lv_obj_center(mbox);
    
    // 添加事件回调
    lv_obj_add_event_cb(mbox, clear_confirm_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

static void clear_confirm_cb(lv_event_t *e)
{
    lv_obj_t *mbox = lv_event_get_current_target(e);
    uint32_t btn_idx = lv_msgbox_get_active_btn(mbox);

    if (btn_idx == 1) {  // Clear按钮被点击
        // 清空购物车
        memset(cart_qty, 0, sizeof(cart_qty));
        
        // 刷新购物车列表
        refresh_cart_list();
    }

    lv_msgbox_close(mbox);
}

void create_scr2(void)
{
    scr2 = lv_obj_create(NULL);
    // 设置暖色调背景（浅橙黄色）
    lv_obj_set_style_bg_color(scr2, lv_color_hex(0xFFDAB9), 0);
    lv_obj_set_style_bg_opa(scr2, LV_OPA_COVER, 0);

    // 添加顶部标题栏
    lv_obj_t *header = lv_obj_create(scr2);
    lv_obj_set_size(header, lv_disp_get_hor_res(NULL) - 140, 50);  // 宽度与cart_list一致
    lv_obj_align(header, LV_ALIGN_TOP_LEFT, 20, 15);  // 与cart_list左对齐
    lv_obj_set_style_bg_color(header, lv_color_hex(0xFF6F00), 0);  // 深橙色渐变起始色
    lv_obj_set_style_bg_grad_color(header, lv_color_hex(0xE65100), 0);  // 深橙色渐变结束色
    lv_obj_set_style_bg_grad_dir(header, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_radius(header, 12, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_shadow_width(header, 8, 0);
    lv_obj_set_style_shadow_color(header, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(header, LV_OPA_30, 0);
    lv_obj_set_style_shadow_ofs_y(header, 3, 0);
    // 禁用header的滚动
    lv_obj_set_scroll_dir(header, LV_DIR_NONE);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Shopping Cart");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);  // 使用已启用的字体
    lv_obj_set_scroll_dir(title, LV_DIR_NONE);
    lv_obj_clear_flag(title, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(title);

    lv_obj_t *btn_cont = lv_obj_create(scr2);
    lv_obj_set_size(btn_cont, 100, 400);  // 增加高度以容纳3个按钮
    lv_obj_align(btn_cont, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_set_style_bg_opa(btn_cont, 0, 0);
    // 禁用按钮容器的滚动
    lv_obj_set_scroll_dir(btn_cont, LV_DIR_NONE);
    lv_obj_clear_flag(btn_cont, LV_OBJ_FLAG_SCROLLABLE);

    // 美化Back按钮
    lv_obj_t *btn_back = lv_btn_create(btn_cont);
    lv_obj_set_size(btn_back, 90, 40);
    lv_obj_align(btn_back, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x757575), 0);
    lv_obj_set_style_bg_grad_color(btn_back, lv_color_hex(0x616161), 0);
    lv_obj_set_style_bg_grad_dir(btn_back, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_radius(btn_back, 10, 0);
    lv_obj_set_style_shadow_width(btn_back, 5, 0);
    lv_obj_set_style_shadow_opa(btn_back, LV_OPA_30, 0);
    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, "Back");
    lv_obj_center(lbl_back);
    lv_obj_add_event_cb(btn_back, switch_btn_event_cb, LV_EVENT_CLICKED, NULL);

    // 添加Clear按钮
    lv_obj_t *btn_clear = lv_btn_create(btn_cont);
    lv_obj_set_size(btn_clear, 90, 40);
    lv_obj_align(btn_clear, LV_ALIGN_CENTER, 0, 0);  // 居中显示
    lv_obj_set_style_bg_color(btn_clear, lv_color_hex(0xFF5722), 0);  // 橙色
    lv_obj_set_style_bg_grad_color(btn_clear, lv_color_hex(0xF4511E), 0);
    lv_obj_set_style_bg_grad_dir(btn_clear, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_radius(btn_clear, 10, 0);
    lv_obj_set_style_shadow_width(btn_clear, 5, 0);
    lv_obj_set_style_shadow_opa(btn_clear, LV_OPA_30, 0);
    lv_obj_t *lbl_clear = lv_label_create(btn_clear);
    lv_label_set_text(lbl_clear, "Clear");
    lv_obj_center(lbl_clear);
    lv_obj_add_event_cb(btn_clear, clear_cart_event_cb, LV_EVENT_CLICKED, NULL);

    // 美化Pay按钮
    lv_obj_t *btn_check = lv_btn_create(btn_cont);
    lv_obj_set_size(btn_check, 90, 40);
    lv_obj_align(btn_check, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(btn_check, lv_color_hex(0x4CAF50), 0);  // 绿色
    lv_obj_set_style_bg_grad_color(btn_check, lv_color_hex(0x43A047), 0);
    lv_obj_set_style_bg_grad_dir(btn_check, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_radius(btn_check, 10, 0);
    lv_obj_set_style_shadow_width(btn_check, 5, 0);
    lv_obj_set_style_shadow_opa(btn_check, LV_OPA_30, 0);
    lv_obj_t *lbl_check = lv_label_create(btn_check);
    lv_label_set_text(lbl_check, "Pay");
    lv_obj_center(lbl_check);
    lv_obj_add_event_cb(btn_check, show_checkout_popup, LV_EVENT_CLICKED, NULL);

    cart_list = lv_obj_create(scr2);
    lv_obj_set_size(cart_list, lv_disp_get_hor_res(NULL) - 140, lv_disp_get_ver_res(NULL) - 120);  // 进一步减小尺寸
    lv_obj_align(cart_list, LV_ALIGN_TOP_LEFT, 20, 85);  // 调整位置，留出更多边距
    // 只允许垂直滚动，禁止水平滚动
    lv_obj_set_scroll_dir(cart_list, LV_DIR_VER);
    lv_obj_clear_flag(cart_list, LV_OBJ_FLAG_SCROLL_CHAIN);
    lv_obj_set_style_pad_all(cart_list, 10, 0);
    // 设置冷色调背景（更冷的浅蓝色），与暖色调页面形成鲜明对比
    lv_obj_set_style_bg_color(cart_list, lv_color_hex(0xBBDEFB), 0);  // 更冷的浅蓝色
    lv_obj_set_style_bg_opa(cart_list, LV_OPA_COVER, 0);
}

static void checkout_msgbox_cb(lv_event_t *e)
{
    lv_obj_t *mbox = lv_event_get_current_target(e);
    uint32_t btn_idx = lv_msgbox_get_active_btn(mbox);

    if (btn_idx == 0) {
        // Confirm按钮被点击，创建新记录并刷新显示
        create_new_record();
        // 直接跳转到交易记录界面
        extern lv_obj_t *scr3;
        lv_scr_load(scr3);
    }

    lv_msgbox_close(mbox);
}

void show_checkout_popup(lv_event_t *e)
{
    // 检查购物车是否为空
    if (is_cart_empty()) {
        lv_obj_t *mbox = lv_msgbox_create(NULL, "Notice", 
                                          "No items to pay in cart.", 
                                          NULL, true);
        lv_obj_center(mbox);
        lv_obj_set_style_bg_color(mbox, lv_color_hex(0xFFF3E0), 0);
        lv_obj_set_style_text_color(lv_msgbox_get_text(mbox), lv_color_hex(0xE65100), 0);
        return;
    }
    
    float total_all = 0;
    for (int i = 0; i < 5; i++) total_all += cart_qty[i] * prices[i];

    char buf[128];
    snprintf(buf, sizeof(buf), "Total: %.2f", total_all);
    static const char *btns[] = {"Confirm", "Cancel", NULL};

    lv_obj_t *mbox = lv_msgbox_create(NULL, "Checkout", buf, btns, true);
    lv_obj_center(mbox);
    lv_obj_add_event_cb(mbox, checkout_msgbox_cb, LV_EVENT_VALUE_CHANGED, NULL);
}
