#include "OrdersAtPrice.h"
#include "Order.h"

OrdersAtPrice::OrdersAtPrice(Price p) : price(p), firstOrder(nullptr), lastOrder(nullptr), orderCount(0), totalQuantity(0) {}

void OrdersAtPrice::appendOrder(Order* order) {
    if (!firstOrder) {
        firstOrder = lastOrder = order;
    } else {
        lastOrder->nextOrder = order;
        order->prevOrder = lastOrder;
        lastOrder = order;
    }
    orderCount++;
    totalQuantity += order->quantity;
}

void OrdersAtPrice::removeOrderFromLevel(Order* order) {
    if (order->prevOrder) {
        order->prevOrder->nextOrder = order->nextOrder;
    } else {
        firstOrder = order->nextOrder;
    }

    if (order->nextOrder) {
        order->nextOrder->prevOrder = order->prevOrder;
    } else {
        lastOrder = order->prevOrder;
    }

    orderCount--;
    totalQuantity -= order->quantity;
}