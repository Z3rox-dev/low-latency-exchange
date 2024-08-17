#include "UDPSocket.h"
#include "logging.h"
#include "logging_util.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <algorithm>

using namespace std::chrono;

Common::Logger logger("client_log.txt");
UDPSocket* client_socket;

void send_udp_message(const std::string& message) {
    auto now = high_resolution_clock::now();
    auto now_us = duration_cast<nanoseconds>(now.time_since_epoch()).count();
    std::string timed_message = std::to_string(now_us) + "," + message;

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1234);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    client_socket->send(timed_message.c_str(), timed_message.length(), server_addr);

    std::array<char, UDPSocket::MAX_BUFFER_SIZE> reply;
    sockaddr_in sender_addr;
    size_t reply_length;
    
    if (client_socket->receive(reply, reply_length, sender_addr)) {
        LOG(info) << "Reply: " << std::string(reply.data(), reply_length);
    }
}

std::string create_order_message(const std::string& line) {
    if (line.empty() || line[0] == '#') {
        return "";
    }

    std::string trimmed_line = line;
    trimmed_line.erase(std::remove(trimmed_line.begin(), trimmed_line.end(), ' '), trimmed_line.end());
    return trimmed_line;
}

void process_csv(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        LOG(error) << "Error opening file: " << file_path;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::string message = create_order_message(line);
        if (!message.empty()) {
            try {
                send_udp_message(message);
            } catch (const std::exception& e) {
                LOG(error) << "Skipping invalid message: " << e.what();
            }
        }
    }
    file.close();
}

int main() {
    initializeLogging();  // Initialize Boost.Log

    client_socket = new UDPSocket(logger);
    if (!client_socket->create("", 0, false)) {
        LOG(error) << "Failed to create client socket";
        return 1;
    }
    client_socket->setNonBlocking();

    std::string csv_file_path = "/workspace/low_latency_engine/TradingEngine/csv/inputFile.csv";
    process_csv(csv_file_path);
    LOG(info) << "All messages sent";

    delete client_socket;
    return 0;
}