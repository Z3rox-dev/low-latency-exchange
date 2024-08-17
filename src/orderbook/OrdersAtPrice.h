#pragma once

#include "Types.h"
#include <cstddef>
class Order;

class OrdersAtPrice {
public:
    explicit OrdersAtPrice(Price p);
    void appendOrder(Order* order);
    void removeOrderFromLevel(Order* order);

    Price price;
    Order* firstOrder;
    Order* lastOrder;
    size_t orderCount;
    Qty totalQuantity;
};