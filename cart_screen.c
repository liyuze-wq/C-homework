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
    const char *paths[5] = 
    {
        "0:/test_image0.bin",
        "0:/test_image1.bin",
        "0:/test_image2.bin",
        "0:/test_image3.bin",
        "0:/test_image4.bin"
    };
    
    int x = 10;
    int y = 10;
    int count = 0;
    static lv_img_dsc_t img_dsc[5];
    
    // 计算右侧图片区域的起始位置
    int screen_width = lv_disp_get_hor_res(NULL);
    int img_area_x = screen_width - 320;  // 右侧留320像素给图片（往左移）
    // 图片高度约97像素（310*80/256≈97），标签高度30像素，让底部对齐
    int img_y = y + 30 - 97;  // 标签Y + 标签高度 - 图片高度

    // 表头
    create_box_label(cart_list, "Product", lv_palette_main(LV_PALETTE_BLUE), x, y);
    create_box_label(cart_list, "Qty", lv_palette_main(LV_PALETTE_ORANGE), x+250, y);
    create_box_label(cart_list, "Total", lv_palette_main(LV_PALETTE_RED), x+450, y);
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
        sprintf(qbuf, "%d.%02d", qty_int, qty_frac);
        
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
        lv_obj_set_pos(img, img_area_x, img_y);

        // 显示商品信息（商品名称与其他标签同一高度）
        create_box_label(cart_list, names[i], lv_palette_lighten(LV_PALETTE_BLUE,2), x, y);
        create_box_label(cart_list, qbuf, lv_palette_lighten(LV_PALETTE_ORANGE,2), x+250, y);
        create_box_label(cart_list, tbuf, lv_palette_lighten(LV_PALETTE_RED,2), x+450, y);

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

void create_scr2(void)
{
    scr2 = lv_obj_create(NULL);

    lv_obj_t *btn_cont = lv_obj_create(scr2);
    lv_obj_set_size(btn_cont, 100, 300);
    lv_obj_align(btn_cont, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_set_style_bg_opa(btn_cont, 0, 0);
    // 禁用按钮容器的滚动
    lv_obj_set_scroll_dir(btn_cont, LV_DIR_NONE);
    lv_obj_clear_flag(btn_cont, LV_OBJ_FLAG_SCROLLABLE);

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
    // 只允许垂直滚动，禁止水平滚动
    lv_obj_set_scroll_dir(cart_list, LV_DIR_VER);
    lv_obj_clear_flag(cart_list, LV_OBJ_FLAG_SCROLL_CHAIN);
    lv_obj_set_style_pad_all(cart_list, 10, 0);
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
    float total_all = 0;
    for (int i = 0; i < 5; i++) total_all += cart_qty[i] * prices[i];

    char buf[128];
    snprintf(buf, sizeof(buf), "Total: %.2f", total_all);
    static const char *btns[] = {"Confirm", "Cancel", NULL};

    lv_obj_t *mbox = lv_msgbox_create(NULL, "Checkout", buf, btns, true);
    lv_obj_center(mbox);
    lv_obj_add_event_cb(mbox, checkout_msgbox_cb, LV_EVENT_VALUE_CHANGED, NULL);
}
