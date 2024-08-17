#include "market_publisher.h"
#include "logging_util.h"
#include <atomic>
#include <thread>

MarketPublisher::MarketPublisher(moodycamel::ConcurrentQueue<MarketData>* queue)
    : marketDataQueue(queue), running(false) {}

void MarketPublisher::run() {
    running = true;
    while (running) {
        MarketData data;
        if (marketDataQueue->try_dequeue(data)) {
            switch (data.type) {
                case MarketData::Type::ADD:
                    LOG(info) << "A, " << data.aggressiveClientId << ", " << data.aggressiveOrderId
                              << (!data.message.empty() ? " (" + data.message + ")" : "");
                    break;
                case MarketData::Type::CANCEL:
                    LOG(info) << "C, " << data.aggressiveClientId << ", " << data.aggressiveOrderId;
                    break;
                case MarketData::Type::TRADE:
                    LOG(info) << "T, " << data.aggressiveClientId << ", " << data.aggressiveOrderId << ", "
                              << data.passiveClientId << ", " << data.passiveOrderId << ", "
                              << data.price << ", " << data.quantity;
                    break;
                case MarketData::Type::BOOK_UPDATE:
                    if (!data.message.empty()) {
                        LOG(info) << data.message;
                    } else {
                        LOG(info) << "B, " << data.side << ", " << data.price << ", " << data.quantity;
                    }
                    break;
                case MarketData::Type::FLUSH:
                    LOG(info) << data.message;
                    break;
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void MarketPublisher::stop() {
    running = false;
}