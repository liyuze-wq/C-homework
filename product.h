#ifndef PRODUCT_H
#define PRODUCT_H

#include "lvgl.h"
#include "gd32h7xx.h"

extern lv_obj_t *scr1;

extern lv_obj_t *scr2;

void btn1_event_cb(lv_event_t *e);

void btn2_event_cb(lv_event_t *e);

typedef struct
{
    char name[20];
    float price;
} Product;

void product_init(void);
Product* get_product_list(void);
int get_product_count(void);

#endif
