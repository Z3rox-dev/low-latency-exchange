#pragma once

#include <array>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include "logging.h"
#include "utils/ErrorCategoryCache.h"

class UDPSocket {
public:
    static constexpr size_t MAX_BUFFER_SIZE = 65507; // Max UDP packet size

    UDPSocket(Common::Logger& logger) : m_logger(logger), m_socket_fd(-1) {}
    ~UDPSocket() { if (m_socket_fd != -1) close(m_socket_fd); }

    bool create(const std::string& ip, int port, bool is_server);
    
    inline bool setNonBlocking() {
        return setNonBlocking(m_socket_fd);
    }
    
    inline bool setSOTimestamp() {
        return setSOTimestamp(m_socket_fd);
    }
    
    bool send(const char* message, size_t length, const sockaddr_in& dest_addr);
    bool receive(std::array<char, MAX_BUFFER_SIZE>& buffer, size_t& received_size, sockaddr_in& sender_addr);

    UDPSocket(const UDPSocket&) = delete;
    UDPSocket& operator=(const UDPSocket&) = delete;
    UDPSocket(UDPSocket&&) = delete;
    UDPSocket& operator=(UDPSocket&&) = delete;

private:
    Common::Logger& m_logger;
    int m_socket_fd;

    static inline bool setNonBlocking(int fd) {
        int flags = fcntl(fd, F_GETFL, 0);
        return flags != -1 && (flags & O_NONBLOCK || fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1);
    }

    static inline bool setSOTimestamp(int fd) {
        int one = 1;
        return (setsockopt(fd, SOL_SOCKET, SO_TIMESTAMP, &one, sizeof(one)) != -1);
    }
};