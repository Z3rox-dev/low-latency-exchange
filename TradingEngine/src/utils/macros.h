#pragma once

#include <cstring>
#include <boost/log/trivial.hpp>
#include <boost/log/sources/severity_logger.hpp>

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

inline auto ASSERT(bool cond, const std::string &msg) noexcept {
  if (UNLIKELY(!cond)) {
    BOOST_LOG_TRIVIAL(fatal) << "ASSERT : " << msg;
    exit(EXIT_FAILURE);
  }
}

inline auto FATAL(const std::string &msg) noexcept {
  BOOST_LOG_TRIVIAL(fatal) << "FATAL : " << msg;
  exit(EXIT_FAILURE);
}