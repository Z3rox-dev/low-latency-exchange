#include "OrderBook.h"
#include "Order.h"
#include <algorithm>
#include <iostream>

OrderBook::OrderBook(TickerId id, moodycamel::ConcurrentQueue<MarketData>* mdQueue, std::size_t initialPoolSize)
    : tickerId(id), 
      nextOrderId(1),
      marketDataQueue(mdQueue), 
      orderPool(initialPoolSize) {}

OrderBook::~OrderBook() {
    for (const auto& pair : orderMap) {
        orderPool.deallocate(pair.second);
    }
}

void OrderBook::addOrder(ClientId clientId, OrderId clientOrderId, Side side, Price price, Qty quantity) {
    auto marketOrderId = nextOrderId++;
    
    Order* order = orderPool.allocate();
    order->clientId = clientId;
    order->clientOrderId = clientOrderId;
    order->marketOrderId = marketOrderId;
    order->tickerId = tickerId;
    order->side = side;
    order->price = price;
    order->quantity = quantity;
    order->priority = 0;
    order->prevOrder = nullptr;
    order->nextOrder = nullptr;

    clientOrderMap[clientOrderId] = std::make_pair(order, side);
    orderMap[marketOrderId] = order;

    MarketData data = {
        MarketData::Type::ADD,
        tickerId,
        clientId,
        clientOrderId,
        0,  // passiveClientId not applicable for ADD
        0,  // passiveOrderId not applicable for ADD
        side == Side::BUY ? 'B' : 'S',
        price,
        quantity,
        ""  // message
    };
    marketDataQueue->enqueue(data);

    if (price == 0) {
        processMarketOrder(order);
    } else {
        if (side == Side::BUY) {
            matchOrder<SellOrderMap>(order, sellOrders.begin(), sellOrders.end());
        } else {
            matchOrder<BuyOrderMap>(order, buyOrders.begin(), buyOrders.end());
        }

        if (order->quantity > 0) {
            if (side == Side::BUY) {
                auto it = buyOrders.find(price);
                if (it == buyOrders.end()) {
                    auto ordersAtPrice = std::make_unique<OrdersAtPrice>(price);
                    ordersAtPrice->appendOrder(order);
                    buyOrders.emplace(price, std::move(ordersAtPrice));
                } else {
                    it->second->appendOrder(order);
                }
            } else {
                auto it = sellOrders.find(price);
                if (it == sellOrders.end()) {
                    auto ordersAtPrice = std::make_unique<OrdersAtPrice>(price);
                    ordersAtPrice->appendOrder(order);
                    sellOrders.emplace(price, std::move(ordersAtPrice));
                } else {
                    it->second->appendOrder(order);
                }
            }
            publishTopOfBook(order, false);
        }
    }
}

void OrderBook::processMarketOrder(Order* order) {
    if (order->side == Side::BUY) {
        matchOrder<SellOrderMap>(order, sellOrders.begin(), sellOrders.end());
    } else {
        matchOrder<BuyOrderMap>(order, buyOrders.begin(), buyOrders.end());
    }
}

bool OrderBook::isBestPrice(const Order* order) const {
    if (order->side == Side::BUY) {
        return buyOrders.empty() || order->price >= buyOrders.rbegin()->first;
    } else {
        return sellOrders.empty() || order->price <= sellOrders.begin()->first;
    }
}

void OrderBook::executeMatch(Order* aggressiveOrder, Order* passiveOrder, Qty matchQty, Price matchPrice) {
    // Update order quantities
    aggressiveOrder->quantity -= matchQty;
    passiveOrder->quantity -= matchQty;

    // Enqueue the trade execution data
    MarketData data = {
        MarketData::Type::TRADE,
        tickerId,
        aggressiveOrder->clientId,
        aggressiveOrder->clientOrderId,
        passiveOrder->clientId,
        passiveOrder->clientOrderId,
        aggressiveOrder->side == Side::BUY ? 'B' : 'S',
        matchPrice,
        matchQty,
        "" 
    };
    marketDataQueue->enqueue(data);

    Side passiveSide = passiveOrder->side;
    if (passiveSide == Side::BUY) {
        auto it = buyOrders.find(passiveOrder->price);
        if (it != buyOrders.end()) {
            it->second->totalQuantity -= matchQty;
            if (it->second->totalQuantity == 0) {
                buyOrders.erase(it);
            }
        }
    } else {
        auto it = sellOrders.find(passiveOrder->price);
        if (it != sellOrders.end()) {
            it->second->totalQuantity -= matchQty;
            if (it->second->totalQuantity == 0) {
                sellOrders.erase(it);
            }
        }
    }
    publishTopOfBook(aggressiveOrder, true);
}

bool OrderBook::cancelOrder(ClientId clientId, OrderId clientOrderId) {
    auto it = clientOrderMap.find(clientOrderId);
    if (it == clientOrderMap.end()) {
        // Enqueue the "not found" cancel order data
        MarketData data = {
            MarketData::Type::CANCEL,
            tickerId,
            clientId,
            clientOrderId,
            0, 0,
            '-',  // side unknown
            0,    // price unknown
            0,    // quantity unknown
            "Not found"
        };
        marketDataQueue->enqueue(data);
        return false;
    }

    Order* orderPtr = it->second.first;
    Side side = it->second.second;

    removeOrderFromBook(orderPtr, side);

    if (side == Side::BUY) {
        auto priceIt = buyOrders.find(orderPtr->price);
        if (priceIt != buyOrders.end() && priceIt->second->orderCount == 0) {
            buyOrders.erase(priceIt);
        }
    } else {
        auto priceIt = sellOrders.find(orderPtr->price);
        if (priceIt != sellOrders.end() && priceIt->second->orderCount == 0) {
            sellOrders.erase(priceIt);
        }
    }

    MarketData data = {
        MarketData::Type::CANCEL,
        tickerId,
        clientId,
        clientOrderId,
        0, 0,
        side == Side::BUY ? 'B' : 'S',
        orderPtr->price,
        orderPtr->quantity,
        ""
    };
    marketDataQueue->enqueue(data);

    publishTopOfBook(orderPtr, false);

    orderPool.deallocate(orderPtr);

    return true;
}


void OrderBook::removeOrderFromBook(Order* order, Side side) {
    if (side == Side::BUY) {
        auto it = buyOrders.find(order->price);
        if (it != buyOrders.end()) {
            it->second->removeOrderFromLevel(order);
        }
    } else {
        auto it = sellOrders.find(order->price);
        if (it != sellOrders.end()) {
            it->second->removeOrderFromLevel(order);
        }
    }
    
    auto clientIt = clientOrderMap.find(order->clientOrderId);
    if (clientIt != clientOrderMap.end()) {
        clientOrderMap.erase(clientIt);
    }
    
    orderMap.erase(order->marketOrderId);
}

void OrderBook::removePriceLevel(Side side, Price price) {
    if (side == Side::BUY) {
        buyOrders.erase(price);
    } else {
        sellOrders.erase(price);
    }
}

void OrderBook::publishTopOfBook(const Order* order, bool isMatch) {
    Side sideToCheck = isMatch ? (order->side == Side::BUY ? Side::SELL : Side::BUY) : order->side;

    if (sideToCheck == Side::BUY) {
        auto it = buyOrders.begin();
        if (it != buyOrders.end()) {
            if (isMatch || order->price >= it->first) {
                MarketData data = {
                    MarketData::Type::BOOK_UPDATE,
                    tickerId,
                    0, 0, 0, 0,
                    'B',
                    it->first,
                    it->second->totalQuantity,
                    ""
                };
                marketDataQueue->enqueue(data);
            }
        } else {
            MarketData data = {
                MarketData::Type::BOOK_UPDATE,
                tickerId,
                0, 0, 0, 0,
                'B',
                0,
                0,
                "B, B, -, -"
            };
            marketDataQueue->enqueue(data);
        }
    } else {
        auto it = sellOrders.begin();
        if (it != sellOrders.end()) {
            if (isMatch || order->price <= it->first) {
                
                MarketData data = {
                    MarketData::Type::BOOK_UPDATE,
                    tickerId,
                    0, 0, 0, 0,
                    'S',
                    it->first,
                    it->second->totalQuantity,
                    ""
                };
                marketDataQueue->enqueue(data);
            }
        } else {
            MarketData data = {
                MarketData::Type::BOOK_UPDATE,
                tickerId,
                0, 0, 0, 0,
                'S',
                0,
                0,
                "B, S, -, -"
            };
            marketDataQueue->enqueue(data);
        }
    }
}

void OrderBook::setTickerId(TickerId id) {
    this->tickerId = id;
}

void OrderBook::reset() {
    nextOrderId = 1;
    buyOrders.clear();
    sellOrders.clear();
    orderMap.clear();
    clientOrderMap.clear();
}

template void OrderBook::matchOrder<OrderBook::BuyOrderMap>(Order* order, OrderBook::BuyOrderMap::iterator begin, OrderBook::BuyOrderMap::iterator end);
template void OrderBook::matchOrder<OrderBook::SellOrderMap>(Order* order, OrderBook::SellOrderMap::iterator begin, OrderBook::SellOrderMap::iterator end);