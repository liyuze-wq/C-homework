#include "lvgl.h"
#include "event.h"


extern void sys_init(void);
extern void lv_port_disp_init(void);
extern void lv_port_indev_init(void);
extern void delay_us(uint32_t ms);

int main(void)
{
    sys_init();

    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();

    create_scr1();
    create_scr2();
    create_scr3_records();  // 使用新的交易记录页面


    lv_scr_load(scr1);

    while (1)
    {
        lv_timer_handler();
        delay_us(5000);
    }
}
