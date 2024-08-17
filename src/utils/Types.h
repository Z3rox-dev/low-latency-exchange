#pragma once

#include <cstdint>

using ClientId = uint32_t;
using OrderId = uint64_t;
using TickerId = uint32_t;
using Price = int32_t;
using Qty = int32_t;

enum class Side : uint8_t {
    BUY,
    SELL
};