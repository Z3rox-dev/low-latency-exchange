cmake_minimum_required(VERSION 3.10)
project(OrderBookProject)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Add profiling flags for all targets
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")

# Include source directories
include_directories(src)
include_directories(${PROJECT_SOURCE_DIR}/src/logger)
include_directories(${PROJECT_SOURCE_DIR}/src/utils) 
include_directories(${PROJECT_SOURCE_DIR}/src/udpsocket)
include_directories(${PROJECT_SOURCE_DIR}/src/matching_engine) 
include_directories(${PROJECT_SOURCE_DIR}/src/message_parser)
include_directories(${PROJECT_SOURCE_DIR}/src/orderbook)
include_directories(${PROJECT_SOURCE_DIR}/src/market_publisher)

# Add the OrderBook library
add_library(OrderBookLib 
    src/orderbook/OrderBook.cpp
    src/orderbook/OrdersAtPrice.cpp
)

# Add the UDPSocket library
add_library(UDPSocketLib
    src/udpsocket/UDPSocket.cpp
)

# Find Boost libraries
find_package(Boost 1.74 REQUIRED COMPONENTS system log log_setup thread filesystem)
include_directories(${Boost_INCLUDE_DIRS})

# Add definition for Boost.Log
add_definitions(-DBOOST_LOG_DYN_LINK)

# Add the logging utility library
add_library(LoggingUtil
    src/logger/logging_util.cpp
)
target_link_libraries(LoggingUtil ${Boost_LIBRARIES})

# Link UDPSocketLib with Boost libraries
target_link_libraries(UDPSocketLib ${Boost_LIBRARIES})

# Link OrderBookLib with Boost libraries
target_link_libraries(OrderBookLib ${Boost_LIBRARIES})

# Add the server executable
add_executable(server 
    src/server.cpp
    src/message_parser/message_parser.cpp
    src/matching_engine/matching_engine.cpp
    src/market_publisher/market_publisher.cpp
)
target_link_libraries(server 
    OrderBookLib 
    UDPSocketLib
    LoggingUtil
    ${Boost_LIBRARIES}
    pthread
)

# Add the client executable
add_executable(client src/client.cpp)
target_link_libraries(client 
    OrderBookLib 
    UDPSocketLib
    LoggingUtil
    ${Boost_LIBRARIES}
    pthread
)

# Set compile options for all targets
target_compile_options(server PRIVATE -Wall -Wextra -pedantic -O3)
target_compile_options(client PRIVATE -Wall -Wextra -pedantic -O3)
target_compile_options(OrderBookLib PRIVATE -Wall -Wextra -pedantic -O3)
target_compile_options(UDPSocketLib PRIVATE -Wall -Wextra -pedantic -O3)
target_compile_options(LoggingUtil PRIVATE -Wall -Wextra -pedantic -O3)