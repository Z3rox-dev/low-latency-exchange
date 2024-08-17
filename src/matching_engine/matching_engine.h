#pragma once

#include "OrderBook.h"
#include "logging.h"
#include "message_parser.h"
#include "utils/concurrentqueue.h"
#include "market_publisher/market_data.h"

extern Common::Logger logger;
extern moodycamel::ConcurrentQueue<MarketData>* marketDataQueue;

void matching_engine();
void initializeMatchingEngine(moodycamel::ConcurrentQueue<MarketData>* queue);