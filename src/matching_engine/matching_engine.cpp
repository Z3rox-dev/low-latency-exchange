#include <chrono>
#include <boost/container/flat_map.hpp>
#include <robin_hood.h>
#include <boost/log/trivial.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include "matching_engine.h"
#include "Types.h"
#include "OrderBook.h"
#include "Order.h"
#include "message_parser.h"
#include "logging_util.h"

using namespace std::chrono;

extern moodycamel::ConcurrentQueue<MarketData>* marketDataQueue;

using Symbol = std::array<char, 16>;

struct SymbolHash {
    size_t operator()(const Symbol& s) const {
        return robin_hood::hash_bytes(s.data(), strnlen(s.data(), s.size()));
    }
};

struct SymbolEqual {
    bool operator()(const Symbol& lhs, const Symbol& rhs) const {
        return strncmp(lhs.data(), rhs.data(), 16) == 0;
    }
};

using SymbolMap = boost::container::flat_map<Symbol, TickerId, std::less<>>;
using OrderBookMap = robin_hood::unordered_flat_map<Symbol, std::unique_ptr<OrderBook>, SymbolHash, SymbolEqual>;

SymbolMap symbolToTickerId;
TickerId nextTickerId = 1;
OrderBookMap neworderBooks;
robin_hood::unordered_flat_map<OrderId, Symbol> orderIdToSymbol;
int testCounter = 1;
const size_t INITIAL_POOL_SIZE = 100000;
const size_t PRE_ALLOCATED_ORDERBOOKS = 20;

std::vector<std::unique_ptr<OrderBook>> orderBookPool;

void initializeOrderBookPool() {
    orderBookPool.reserve(PRE_ALLOCATED_ORDERBOOKS);
    for (size_t i = 0; i < PRE_ALLOCATED_ORDERBOOKS; ++i) {
        orderBookPool.push_back(std::make_unique<OrderBook>(0, marketDataQueue, INITIAL_POOL_SIZE));
    }
}

std::unique_ptr<OrderBook> getOrderBook(TickerId id) {
    if (!orderBookPool.empty()) {
        auto orderBook = std::move(orderBookPool.back());
        orderBookPool.pop_back();
        orderBook->setTickerId(id);
        orderBook->reset();
        return orderBook;
    }
    return std::make_unique<OrderBook>(id, marketDataQueue, INITIAL_POOL_SIZE);
}

void processMessage(const ParsedMessage& msg) {
    auto start_process_time = high_resolution_clock::now();

    Symbol symbol;
    std::strncpy(symbol.data(), msg.symbol.c_str(), symbol.size() - 1);
    symbol[symbol.size() - 1] = '\0';

    if (msg.type == "N") {
        auto it = symbolToTickerId.find(symbol);
        if (it == symbolToTickerId.end()) {
            it = symbolToTickerId.emplace(symbol, nextTickerId++).first;
        }
        
        auto& orderBook = neworderBooks[symbol];
        if (!orderBook) {
            orderBook = getOrderBook(it->second);
        }

        orderBook->addOrder(msg.userId, msg.userOrderId, 
                            msg.side == 'B' ? Side::BUY : Side::SELL, 
                            msg.price, msg.quantity);
        
        orderIdToSymbol.emplace(msg.userOrderId, symbol);
    }
    else if (msg.type == "C") {
        auto symbolIt = orderIdToSymbol.find(msg.userOrderId);
        if (symbolIt != orderIdToSymbol.end()) {
            auto& orderBook = neworderBooks[symbolIt->second];
            if (orderBook->cancelOrder(msg.userId, msg.userOrderId)) {
                orderIdToSymbol.erase(symbolIt);
            }
        } else {
            BOOST_LOG_SEV(g_logger, boost::log::trivial::info) << "C, " << msg.userId << ", " << msg.userOrderId << " (Not found in any book)";
        }
    } else if (msg.type == "F") { 
        for (auto& [symbol, orderBook] : neworderBooks) {
            orderBook->reset();
            orderBookPool.push_back(std::move(orderBook));
        }
        neworderBooks.clear();
        symbolToTickerId.clear();
        orderIdToSymbol.clear();
        nextTickerId = 1;

        marketDataQueue->enqueue(MarketData{
            MarketData::Type::FLUSH, 0, 0, 0, 0, 0, '-', 0, 0,
            "Book Flush Test #" + std::to_string(testCounter++)
        });
    }

    auto end_process_time = high_resolution_clock::now();
    auto processing_duration = duration_cast<nanoseconds>(end_process_time - start_process_time).count();
    auto network_latency_us = duration_cast<nanoseconds>(msg.receiveTime.time_since_epoch()).count() - msg.sendTimeUs;
    auto total_latency_us = processing_duration + network_latency_us;

    logger.logLatency(msg.type, network_latency_us, processing_duration, total_latency_us);
}

void matching_engine() {
    initializeOrderBookPool();
    ParsedMessage msg;
    while (true) {
        if (parsedMessageQueue.try_dequeue(msg)) {
            processMessage(msg);
        }
    }
}

void initializeMatchingEngine(moodycamel::ConcurrentQueue<MarketData>* queue) {
    marketDataQueue = queue;
    initializeOrderBookPool();
}