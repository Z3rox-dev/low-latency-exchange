#pragma once

#include "market_data.h"
#include "utils/concurrentqueue.h"

class MarketPublisher {
public:
    MarketPublisher(moodycamel::ConcurrentQueue<MarketData>* queue);
    void run();
    void stop();

private:
    moodycamel::ConcurrentQueue<MarketData>* marketDataQueue;
    bool running;
};