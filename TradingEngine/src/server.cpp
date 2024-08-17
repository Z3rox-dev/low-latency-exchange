#include "UDPSocket.h"
#include "logging.h"
#include "utils/concurrentqueue.h"
#include "message_parser.h"
#include "matching_engine.h"
#include "market_publisher/market_publisher.h"
#include "market_publisher/market_data.h"
#include <string>
#include <thread>
#include <chrono>
#include "utils/OptMemPool.h" 
#include "logging_util.h"

using namespace std::chrono;

constexpr size_t QUEUE_SIZE = 100000;

extern moodycamel::ConcurrentQueue<std::pair<std::string, system_clock::time_point>> rawMessageQueue;

OptCommon::OptMemPool<moodycamel::ConcurrentQueue<MarketData>>* marketDataQueuePool = nullptr;
moodycamel::ConcurrentQueue<MarketData>* marketDataQueue = nullptr;

Common::Logger logger("server_log.txt");

UDPSocket* server_socket;

void initializeGlobalData() {
    marketDataQueuePool = new OptCommon::OptMemPool<moodycamel::ConcurrentQueue<MarketData>>(2);
    marketDataQueue = marketDataQueuePool->allocate(QUEUE_SIZE);
}

void cleanupGlobalData() {
    if (marketDataQueue && marketDataQueuePool) {
        marketDataQueuePool->deallocate(marketDataQueue);
        delete marketDataQueuePool;
        marketDataQueue = nullptr;
        marketDataQueuePool = nullptr;
    }
}

void udp_server(const std::string& host, int port) {
    server_socket = new UDPSocket(logger);
    if (!server_socket->create(host, port, true)) {
        LOG(error) << "Failed to create server socket";
        return;
    }
    server_socket->setNonBlocking();
    server_socket->setSOTimestamp();

    std::array<char, UDPSocket::MAX_BUFFER_SIZE> buffer;
    LOG(info) << "Server started and listening on " << host << ":" << port;

    while (true) {
        sockaddr_in client_endpoint;
        size_t received_size;
        if (server_socket->receive(buffer, received_size, client_endpoint)) {
            auto receive_time = high_resolution_clock::now();
            std::string message(buffer.data(), received_size);

            rawMessageQueue.enqueue(std::make_pair(message, receive_time));
        }
    }
}

int main() {
    initializeLogging();
    initializeGlobalData();

    MarketPublisher marketPublisher(marketDataQueue);
    initializeMatchingEngine(marketDataQueue);

    // Define core IDs for each component
    int server_core_id = 0;
    int parser_core_id = 1;
    int engine_core_id = 2;
    int publisher_core_id = 3;

    // Create and start threads with core affinity
    auto server_thread = Common::createAndStartThread(server_core_id, "UDPServer", 
        []() { udp_server("127.0.0.1", 1234); });

    auto parser_thread = Common::createAndStartThread(parser_core_id, "MessageParser", 
        message_parser);

    auto engine_thread = Common::createAndStartThread(engine_core_id, "MatchingEngine", 
        matching_engine);

    auto publisher_thread = Common::createAndStartThread(publisher_core_id, "MarketPublisher", 
        [&marketPublisher]() { marketPublisher.run(); });

    server_thread->join();
    parser_thread->join();
    engine_thread->join();
    publisher_thread->join();

    // Clean up resources
    delete server_thread;
    delete parser_thread;
    delete engine_thread;
    delete publisher_thread;

    cleanupGlobalData();
    delete server_socket;
    return 0;
}