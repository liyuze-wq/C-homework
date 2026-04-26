#ifndef CART_SCREEN_H
#define CART_SCREEN_H

#include "lvgl.h"

extern lv_obj_t *scr2;
extern float cart_qty[5];

void create_scr2(void);
void refresh_cart_list(void);
void show_checkout_popup(lv_event_t *e);

#endif /* CART_SCREEN_H */
