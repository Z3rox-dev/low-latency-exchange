#pragma once

#include <boost/container/flat_map.hpp>
#include <unordered_map>
#include "Types.h"
#include "OrdersAtPrice.h"
#include "Order.h"
#include "utils/concurrentqueue.h"
#include "market_publisher/market_data.h"
#include "OptMemPool.h"
#include "robin_hood.h" 

class OrderBook {
public:
    using BuyOrderMap = boost::container::flat_map<Price, std::unique_ptr<OrdersAtPrice>, std::greater<Price>>;
    using SellOrderMap = boost::container::flat_map<Price, std::unique_ptr<OrdersAtPrice>, std::less<Price>>;

    OrderBook(TickerId id, moodycamel::ConcurrentQueue<MarketData>* marketDataQueue, std::size_t initialPoolSize);
    ~OrderBook();
    
    void addOrder(ClientId clientId, OrderId clientOrderId, Side side, Price price, Qty quantity);
    bool cancelOrder(ClientId clientId, OrderId clientOrderId);

void setTickerId(TickerId id);

void reset();

template<typename OrderMap>
void matchOrder(Order* order, typename OrderMap::iterator begin, typename OrderMap::iterator end) {
    auto it = begin;
    while (it != end && order->quantity > 0) {
        auto currentIt = it++; 
        auto& ordersAtPrice = *(currentIt->second);
        
        if (order->price != 0 && 
            ((order->side == Side::BUY && order->price < ordersAtPrice.price) ||
             (order->side == Side::SELL && order->price > ordersAtPrice.price))) {
            break;
        }

        auto matchingOrder = ordersAtPrice.firstOrder;
        while (matchingOrder && order->quantity > 0) {
            auto matchQty = std::min(order->quantity, matchingOrder->quantity);
            executeMatch(order, matchingOrder, matchQty, ordersAtPrice.price);

            if (matchingOrder->quantity == 0) {
                auto nextOrder = matchingOrder->nextOrder;
                removeOrderFromBook(matchingOrder, order->side == Side::BUY ? Side::SELL : Side::BUY);
                matchingOrder = nextOrder;
            } else {
                matchingOrder = matchingOrder->nextOrder;
            }
        }

        if (ordersAtPrice.orderCount == 0) {
            if constexpr (std::is_same_v<OrderMap, BuyOrderMap>) {
                buyOrders.erase(currentIt);
            } else {
                sellOrders.erase(currentIt);
            }
        }
    }
}
    
private:
    TickerId tickerId;
    OrderId nextOrderId;
    moodycamel::ConcurrentQueue<MarketData>* marketDataQueue;
    OptCommon::OptMemPool<Order> orderPool;

    BuyOrderMap buyOrders;
    SellOrderMap sellOrders;
    robin_hood::unordered_map<OrderId, Order*> orderMap;
    robin_hood::unordered_map<OrderId, std::pair<Order*, Side>> clientOrderMap;
    
    void removeOrderFromBook(Order* order, Side side);
    void publishTopOfBook(const Order* order, bool isMatch = false);
    void removePriceLevel(Side side, Price price);
    void executeMatch(Order* aggressiveOrder, Order* passiveOrder, Qty matchQty, Price matchPrice);
    void processMarketOrder(Order* order);
    bool isBestPrice(const Order* order) const;
};