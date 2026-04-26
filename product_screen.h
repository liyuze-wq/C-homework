#ifndef PRODUCT_SCREEN_H
#define PRODUCT_SCREEN_H

#include "lvgl.h"

extern lv_obj_t *scr1;
extern float cart_qty[5];

void create_scr1(void);
void kb_event_cb(lv_event_t *e);
void update_total_sum(void);

#endif /* PRODUCT_SCREEN_H */
