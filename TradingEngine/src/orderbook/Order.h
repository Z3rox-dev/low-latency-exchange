#pragma once

#include "Types.h"
#include <iostream>

struct Order {
    ClientId clientId;
    OrderId clientOrderId;
    OrderId marketOrderId;
    TickerId tickerId;
    Side side;
    Price price;
    Qty quantity;
    int priority;
    Order* prevOrder;
    Order* nextOrder;
};