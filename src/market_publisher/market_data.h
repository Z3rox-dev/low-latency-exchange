#pragma once

#include "Types.h"
#include "utils/concurrentqueue.h"
#include "market_data.h"

struct MarketData {
    enum class Type { ADD, CANCEL, TRADE, BOOK_UPDATE, FLUSH };
    Type type;
    TickerId tickerId;
    ClientId aggressiveClientId;
    OrderId aggressiveOrderId;
    ClientId passiveClientId;  
    OrderId passiveOrderId;
    char side;
    Price price;
    Qty quantity;
    std::string message;
};

extern moodycamel::ConcurrentQueue<MarketData>* marketDataQueue;