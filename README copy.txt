Low-Latency Exchange Simulation
Overview
This project simulates a small-scale, high-performance exchange focused on low latency and fast order execution. Implemented in C++20, it demonstrates the architecture and capabilities of a modern trading system.
Key Features

Multi-threaded Architecture: Concurrent processing through multiple threads (matching engine, order book, market publisher, server, client simulation).
Low-Latency Design: ~100 nanoseconds average processing time per order (excluding network latency).
Efficient Memory Management: Custom allocator and memory pools.
High-Performance Data Structures: Lock-free concurrent queues, boost::container::flat_map for order book management.
Optimized Matching Algorithm: Price-time priority with template metaprogramming optimizations.
Robust Order Management: Supports market and limit orders, efficient cancellation and modification.
Real-time Market Data Publishing: Low-latency updates on trades and order book changes.
Simulated Network Communication: UDP server for order reception and client simulation.

Stack

Language: C++20
Build System: CMake
Dependencies: Boost, Google
Containerization: Docker support

Performance Metrics

Order Processing Time: ~100 nanoseconds (average, excluding network latency)
Network Latency: ~1000 nanoseconds (due to infrastructure limitations)
Throughput: Capable of handling thousands of orders per second

Getting Started
Prerequisites

C++20 compatible compiler (e.g., GCC 10+, Clang 10+)
CMake 3.15+
Boost libraries
