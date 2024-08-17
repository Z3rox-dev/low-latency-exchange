#include "UDPSocket.h"
#include "ErrorCategoryCache.h"

bool UDPSocket::create(const std::string& ip, int port, bool is_server) {
    m_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_socket_fd == -1) {
        m_logger.log("Failed to create socket. Error: %\n", Utils::ErrorCategoryCache::getSystemCategory().message(errno));
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = ip.empty() ? INADDR_ANY : inet_addr(ip.c_str());

    if (is_server && bind(m_socket_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1) {
        m_logger.log("Failed to bind socket. Error: %\n", Utils::ErrorCategoryCache::getSystemCategory().message(errno));
        close(m_socket_fd);
        m_socket_fd = -1;
        return false;
    }

    return true;
}

bool UDPSocket::send(const char* message, size_t length, const sockaddr_in& dest_addr) {
    ssize_t sent = sendto(m_socket_fd, message, length, 0,
        reinterpret_cast<const struct sockaddr*>(&dest_addr), sizeof(dest_addr));
    if (sent == -1) {
        m_logger.log("Failed to send message. Error: %\n", Utils::ErrorCategoryCache::getSystemCategory().message(errno));
        return false;
    }
    return true;
}

bool UDPSocket::receive(std::array<char, MAX_BUFFER_SIZE>& buffer, size_t& received_size, sockaddr_in& sender_addr) {
    socklen_t sender_addr_len = sizeof(sender_addr);
    ssize_t received = recvfrom(m_socket_fd, buffer.data(), buffer.size(), 0,
        reinterpret_cast<struct sockaddr*>(&sender_addr), &sender_addr_len);
    if (received == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            m_logger.log("Failed to receive message. Error: %\n", Utils::ErrorCategoryCache::getSystemCategory().message(errno));
        }
        return false;
    }
    received_size = static_cast<size_t>(received);
    return true;
}