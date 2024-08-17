#pragma once

#include <string>
#include <chrono>
#include "utils/concurrentqueue.h"

struct ParsedMessage {
    long long sendTimeUs;
    std::chrono::system_clock::time_point receiveTime;
    std::string type;
    int userId;
    std::string symbol;
    int price;
    int quantity;
    char side;
    int userOrderId;

    ParsedMessage() {
        // Preallocare spazio per il simbolo, assumendo una lunghezza massima di 16 caratteri
        symbol.reserve(6);
    }
};

constexpr size_t MAX_PARTS = 10;
constexpr size_t INITIAL_QUEUE_SIZE = 100000;

extern moodycamel::ConcurrentQueue<ParsedMessage> parsedMessageQueue;
extern moodycamel::ConcurrentQueue<std::pair<std::string, std::chrono::system_clock::time_point>> rawMessageQueue;

void parseMessage(const std::string& message, std::chrono::system_clock::time_point receive_time);
void message_parser();
ParsedMessage dequeue_parsed_message();