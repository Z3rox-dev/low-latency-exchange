#include "message_parser.h"
#include "logging_util.h"
#include <array>
#include <string_view>
#include <charconv>
#include <cstring>

moodycamel::ConcurrentQueue<ParsedMessage> parsedMessageQueue(INITIAL_QUEUE_SIZE);
moodycamel::ConcurrentQueue<std::pair<std::string, std::chrono::system_clock::time_point>> rawMessageQueue(INITIAL_QUEUE_SIZE);

thread_local moodycamel::ProducerToken producer(parsedMessageQueue);
thread_local moodycamel::ConsumerToken consumer(rawMessageQueue);

inline void parseMessage(const std::string& message, std::chrono::system_clock::time_point receive_time) {
    std::array<std::string_view, MAX_PARTS> parts;
    size_t part_count = 0;
    
    const char* start = message.data();
    const char* end = start;
    const char* msg_end = start + message.length();

    while (end < msg_end && part_count < MAX_PARTS) {
        end = static_cast<const char*>(memchr(start, ',', msg_end - start));
        if (!end) {
            end = msg_end;
        }
        parts[part_count++] = std::string_view(start, end - start);
        start = end + 1;
    }

    if (part_count == 0 || parts[0].front() == '#') {
        return;
    }

    ParsedMessage parsedMsg;
    parsedMsg.receiveTime = receive_time;

    std::from_chars(parts[0].data(), parts[0].data() + parts[0].size(), parsedMsg.sendTimeUs);
    parsedMsg.type = parts[1];

    switch (parsedMsg.type.front()) {
        case 'N':
            if (part_count == 8) {
                std::from_chars(parts[2].data(), parts[2].data() + parts[2].size(), parsedMsg.userId);
                parsedMsg.symbol = parts[3];
                std::from_chars(parts[4].data(), parts[4].data() + parts[4].size(), parsedMsg.price);
                std::from_chars(parts[5].data(), parts[5].data() + parts[5].size(), parsedMsg.quantity);
                parsedMsg.side = parts[6].front();
                std::from_chars(parts[7].data(), parts[7].data() + parts[7].size(), parsedMsg.userOrderId);
            } else {
                LOG(warning) << "Invalid 'N' message format";
                return;
            }
            break;
        case 'C':
            if (part_count == 4) {
                std::from_chars(parts[2].data(), parts[2].data() + parts[2].size(), parsedMsg.userId);
                std::from_chars(parts[3].data(), parts[3].data() + parts[3].size(), parsedMsg.userOrderId);
            } else {
                LOG(warning) << "Invalid 'C' message format";
                return;
            }
            break;
        case 'F':
            break;
        default:
            LOG(warning) << "Unknown message type: " << parsedMsg.type;
            return;
    }

    parsedMessageQueue.enqueue(producer, std::move(parsedMsg));
}

void message_parser() {
    std::pair<std::string, std::chrono::system_clock::time_point> message_info;
    while (true) {
        while (rawMessageQueue.try_dequeue(consumer, message_info)) {
            parseMessage(message_info.first, message_info.second);
        }
    }
}

ParsedMessage dequeue_parsed_message() {
    ParsedMessage msg;
    while (!parsedMessageQueue.try_dequeue(consumer, msg)) {
        // Spin-wait
    }
    return msg;
}