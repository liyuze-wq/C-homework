#include "product.h"
#include <string.h>

static Product product_list[5];
static int product_count = 5;

void product_init(void)
{
    strcpy(product_list[0].name, "milk");
    product_list[0].price = 5.0f;

    strcpy(product_list[1].name, "bread");
    product_list[1].price = 6.5f;

    strcpy(product_list[2].name, "apple");
    product_list[2].price = 8.0f;

    strcpy(product_list[3].name, "water");
    product_list[3].price = 3.0f;

    strcpy(product_list[4].name, "chips");
    product_list[4].price = 7.5f;
}

Product* get_product_list(void)
{
    return product_list;
}

int get_product_count(void)
{
    return product_count;
}
